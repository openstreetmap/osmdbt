/* OSM Logical Replication
 *
 * Copyright 2017 Matt Amos <zerebubuth@gmail.com>
 * Released under the Apache-2.0 license. Please see LICENSE.md for details.
 *
 * Partly based on https://github.com/postgres/postgres/blob/master/contrib/test_decoding/test_decoding.c
 */

#include "postgres.h"

#include "catalog/pg_type.h"

#include "replication/logical.h"
#include "replication/origin.h"
#include "replication/output_plugin.h"

#include "utils/memutils.h"
#include "utils/rel.h"

#include <inttypes.h>

PG_MODULE_MAGIC;

extern void _PG_init(void);
extern void _PG_output_plugin_init(OutputPluginCallbacks *);

typedef struct
{
   MemoryContext context;
   bool          xact_needs_commit;
} OsmLogicalData;

static void pg_osmlogical_startup(
  LogicalDecodingContext *ctx,
  OutputPluginOptions *opt,
  bool is_init);

static void pg_osmlogical_shutdown(
  LogicalDecodingContext *ctx);

static void pg_osmlogical_begin(
  LogicalDecodingContext *ctx,
  ReorderBufferTXN *txn);

static void pg_osmlogical_change(
  LogicalDecodingContext *ctx,
  ReorderBufferTXN *txn,
  Relation rel,
  ReorderBufferChange *change);

static void pg_osmlogical_commit(
  LogicalDecodingContext *ctx,
  ReorderBufferTXN *txn,
  XLogRecPtr commit_lsn);

void _PG_init(void) {
}

void _PG_output_plugin_init(OutputPluginCallbacks *cb) {
  AssertVariableIsOfType(&_PG_output_plugin_init, LogicalOutputPluginInit);

  cb->startup_cb = pg_osmlogical_startup;
  cb->begin_cb = pg_osmlogical_begin;
  cb->change_cb = pg_osmlogical_change;
  cb->commit_cb = pg_osmlogical_commit;
  cb->shutdown_cb = pg_osmlogical_shutdown;
}

static void pg_osmlogical_startup(
  LogicalDecodingContext *ctx,
  OutputPluginOptions *opt,
  bool is_init) {

  OsmLogicalData *data;

  data = palloc0(sizeof(OsmLogicalData));
  data->context = AllocSetContextCreate(ctx->context,  "osmlogical-context",
#if PG_VERSION_NUM >= 110000
						       ALLOCSET_DEFAULT_SIZES);
#else
                                                       ALLOCSET_SMALL_MINSIZE,
                                                       ALLOCSET_SMALL_INITSIZE,
                                                       ALLOCSET_SMALL_MAXSIZE);
#endif

  data->xact_needs_commit = false;

  ctx->output_plugin_private = data;

  opt->output_type = OUTPUT_PLUGIN_TEXTUAL_OUTPUT;
}


static void pg_osmlogical_shutdown(LogicalDecodingContext *ctx) {

  OsmLogicalData *data = ctx->output_plugin_private;

  // cleanup our own resources via memory context reset
  MemoryContextDelete(data->context);
}

static void pg_osmlogical_begin(
  LogicalDecodingContext *ctx,
  ReorderBufferTXN *txn) {

  OsmLogicalData *data = ctx->output_plugin_private;
  data->xact_needs_commit = false;
}

static void append_bigint(StringInfo out, Datum val) {
  appendStringInfo(out, "%" PRIu64, DatumGetInt64(val));
}

static Datum get_attribute_by_name(
  HeapTuple tuple,
  TupleDesc desc,
  const char *name,
  bool *is_null) {

  int i;

  for (i = 0; i < desc->natts; i++) {
#if PG_VERSION_NUM < 110000
    if (strcmp(NameStr(desc->attrs[i]->attname), name) == 0) {
#else
    if (strcmp(NameStr(desc->attrs[i].attname), name) == 0) {
#endif
      return heap_getattr(tuple, i + 1, desc, is_null);
    }
  }

  *is_null = true;
  return (Datum) NULL;
}

static void pg_osmlogical_change(
  LogicalDecodingContext *ctx,
  ReorderBufferTXN *txn,
  Relation rel,
  ReorderBufferChange *change) {

  Form_pg_class form;
  TupleDesc desc;
  bool is_null_id = false;
  bool is_null_version = false;
  bool is_null_cid = false;
  bool is_null_redaction = false;
  Datum id, version, cid, redaction;
  const char *table_name;
  HeapTuple tuple;
  char *id_name;
  char table_type;
  OsmLogicalData *data;
  MemoryContext old;

  AssertVariableIsOfType(&pg_osmlogical_change, LogicalDecodeChangeCB);

  // Skip DELETE action (not relevant for OSM tables)
  if (change->action != REORDER_BUFFER_CHANGE_INSERT &&
      change->action != REORDER_BUFFER_CHANGE_UPDATE) {
    return;
  }

  form = RelationGetForm(rel);
  table_name = NameStr(form->relname);

  // Which table is this about? Ignore all changes for tables other than
  // the OSM object tables.
  if (strncmp(table_name, "nodes", 6) == 0) {
    table_type = 'n';
    id_name = "node_id";
  } else if (strncmp(table_name, "ways", 5) == 0) {
    table_type = 'w';
    id_name = "way_id";
  } else if (strncmp(table_name, "relations", 10) == 0) {
    table_type = 'r';
    id_name = "relation_id";
  } else {
    return;
  }

  data = ctx->output_plugin_private;

  // Avoid leaking memory by using and resetting our own context
  old = MemoryContextSwitchTo(data->context);

  // Sanity checks
  switch (change->action) {

    case REORDER_BUFFER_CHANGE_INSERT:
      if (change->data.tp.newtuple == NULL) {
         elog(WARNING, "no tuple data for INSERT in table \"%s\"", table_name);
         MemoryContextSwitchTo(old);
         MemoryContextReset(data->context);
         return;
      }
      break;

    case REORDER_BUFFER_CHANGE_UPDATE:
      if (change->data.tp.newtuple == NULL) {
         elog(WARNING, "no tuple data for UPDATE in table \"%s\"", table_name);
         MemoryContextSwitchTo(old);
         MemoryContextReset(data->context);
         return;
      }
      break;

    case REORDER_BUFFER_CHANGE_DELETE:
      break;

    default:
      Assert(false);
  }

  // Get object id and version.
  desc = RelationGetDescr(rel);
  tuple = &change->data.tp.newtuple->tuple;
  id = get_attribute_by_name(tuple, desc, id_name, &is_null_id);
  version = get_attribute_by_name(tuple, desc, "version", &is_null_version);
  cid = get_attribute_by_name(tuple, desc, "changeset_id", &is_null_cid);

  // Paranoia check.
  if (is_null_id || is_null_version || is_null_cid) {
    elog(WARNING, "NULL id, version or changeset id in table \"%s\"", table_name);

    MemoryContextSwitchTo(old);
    MemoryContextReset(data->context);
    return;
  }

  data->xact_needs_commit = true;

  switch (change->action) {

    // New version of an OSM object.
    case REORDER_BUFFER_CHANGE_INSERT:
  
      OutputPluginPrepareWrite(ctx, true);
      appendStringInfo(ctx->out, "N %c", table_type);
      append_bigint(ctx->out, id);
      appendStringInfoString(ctx->out, " v");
      append_bigint(ctx->out, version);
      appendStringInfoString(ctx->out, " c");
      append_bigint(ctx->out, cid);
      break;

    // Updated element with redaction_id
    case REORDER_BUFFER_CHANGE_UPDATE:

      redaction = get_attribute_by_name(tuple, desc, "redaction_id", &is_null_redaction);
      OutputPluginPrepareWrite(ctx, true);

      if (is_null_redaction) {
        appendStringInfo(ctx->out, "UPDATE with redaction_id set to NULL for %c", table_type);
        append_bigint(ctx->out, id);
        appendStringInfoString(ctx->out, " v");
        append_bigint(ctx->out, version);
        appendStringInfoString(ctx->out, " c");
        append_bigint(ctx->out, cid);
      } else {
        appendStringInfo(ctx->out, "R %c", table_type);
        append_bigint(ctx->out, id);
        appendStringInfoString(ctx->out, " v");
        append_bigint(ctx->out, version);
        appendStringInfoString(ctx->out, " c");
        append_bigint(ctx->out, cid);
        appendStringInfoString(ctx->out, " ");
        append_bigint(ctx->out, redaction);
      }
      break;

    // No delete actions on OSM tables
    case REORDER_BUFFER_CHANGE_DELETE:
      break;

    default:
      Assert(false);
  }

  MemoryContextSwitchTo(old);
  MemoryContextReset(data->context);
  OutputPluginWrite(ctx, true);
}

static void pg_osmlogical_commit(
  LogicalDecodingContext *ctx,
  ReorderBufferTXN *txn,
  XLogRecPtr commit_lsn) {

  OsmLogicalData *data = ctx->output_plugin_private;

  if (!data->xact_needs_commit) {
    return;
  }

  data->xact_needs_commit = false;

  OutputPluginPrepareWrite(ctx, true);
  appendStringInfoString(ctx->out, "C");
  OutputPluginWrite(ctx, true);
}

