/* OSM Logical Replication
 *
 * Copyright 2017 Matt Amos <zerebubuth@gmail.com>
 * Released under the Apache-2.0 license. Please see LICENSE.md for details.
 */

#include "postgres.h"

#include "catalog/pg_type.h"

#include "replication/logical.h"
#include "replication/origin.h"
#include "replication/output_plugin.h"

#include "utils/rel.h"

#include <inttypes.h>

PG_MODULE_MAGIC;

extern void _PG_init(void);
extern void _PG_output_plugin_init(OutputPluginCallbacks *);

static void startup(
  LogicalDecodingContext *ctx,
  OutputPluginOptions *opt,
  bool is_init);

static void begin(
  LogicalDecodingContext *ctx,
  ReorderBufferTXN *txn);

static void change(
  LogicalDecodingContext *ctx,
  ReorderBufferTXN *txn,
  Relation rel,
  ReorderBufferChange *change);

static void commit(
  LogicalDecodingContext *ctx,
  ReorderBufferTXN *txn,
  XLogRecPtr commit_lsn);

void _PG_init(void) {
}

void _PG_output_plugin_init(OutputPluginCallbacks *cb) {
  AssertVariableIsOfType(&_PG_output_plugin_init, LogicalOutputPluginInit);

  cb->startup_cb = startup;
  cb->begin_cb = begin;
  cb->change_cb = change;
  cb->commit_cb = commit;
}

static void startup(
  LogicalDecodingContext *ctx,
  OutputPluginOptions *opt,
  bool is_init) {

  opt->output_type = OUTPUT_PLUGIN_TEXTUAL_OUTPUT;
}

void begin(
  LogicalDecodingContext *ctx,
  ReorderBufferTXN *txn) {

  OutputPluginPrepareWrite(ctx, true);
  appendStringInfoString(ctx->out, "B");
  OutputPluginWrite(ctx, true);
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

void change(
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

  // Paranoia check.
  if (change->data.tp.newtuple == NULL) {
    OutputPluginPrepareWrite(ctx, true);
    appendStringInfoString(ctx->out, "X newtuple is NULL");
    OutputPluginWrite(ctx, true);
    return;
  }

  // Get object id and version.
  desc = RelationGetDescr(rel);
  tuple = &change->data.tp.newtuple->tuple;
  id = get_attribute_by_name(tuple, desc, id_name, &is_null_id);
  version = get_attribute_by_name(tuple, desc, "version", &is_null_version);
  cid = get_attribute_by_name(tuple, desc, "changeset_id", &is_null_cid);

  // Paranoia check.
  if (is_null_id || is_null_version || is_null_cid) {
    OutputPluginPrepareWrite(ctx, true);
    appendStringInfoString(ctx->out, "X NULL id or version");
    OutputPluginWrite(ctx, true);
    return;
  }

  // New version of an OSM object.
  if (change->action == REORDER_BUFFER_CHANGE_INSERT) {
    OutputPluginPrepareWrite(ctx, true);
    appendStringInfo(ctx->out, "N %c", table_type);
    append_bigint(ctx->out, id);
    appendStringInfoString(ctx->out, " v");
    append_bigint(ctx->out, version);
    appendStringInfoString(ctx->out, " c");
    append_bigint(ctx->out, cid);
    OutputPluginWrite(ctx, true);
    return;
  }

  // Updated element with redaction_id
  if (change->action == REORDER_BUFFER_CHANGE_UPDATE) {
    redaction = get_attribute_by_name(tuple, desc, "redaction_id", &is_null_redaction);
    OutputPluginPrepareWrite(ctx, true);
    if (is_null_redaction) {
      appendStringInfo(ctx->out, "UPDATE with redaction_id set to NULL for %c", table_type);
      append_bigint(ctx->out, id);
      appendStringInfoString(ctx->out, " v");
      append_bigint(ctx->out, version);
      appendStringInfoString(ctx->out, " c");
      append_bigint(ctx->out, cid);
      OutputPluginWrite(ctx, true);
      return;
    }
    appendStringInfo(ctx->out, "R %c", table_type);
    append_bigint(ctx->out, id);
    appendStringInfoString(ctx->out, " v");
    append_bigint(ctx->out, version);
    appendStringInfoString(ctx->out, " c");
    append_bigint(ctx->out, cid);
    appendStringInfoString(ctx->out, " ");
    append_bigint(ctx->out, redaction);
    OutputPluginWrite(ctx, true);
  }
}

void commit(
  LogicalDecodingContext *ctx,
  ReorderBufferTXN *txn,
  XLogRecPtr commit_lsn) {

  OutputPluginPrepareWrite(ctx, true);
  appendStringInfoString(ctx->out, "C");
  OutputPluginWrite(ctx, true);
}

