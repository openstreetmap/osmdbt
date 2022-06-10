
# PostgreSQL logical decoding plugin

This is the plugin needed for osmdbt operations. It must be installed into
the plugin search path of PostgreSQL.

This was originally developed by Matt Amos at
https://github.com/zerebubuth/osm-logical .

## Testing

To run a test of the replication plugin you'll need ruby and the ruby-pg
package. It will only work if the plugin is already installed somewhere where
PostgreSQL can find it.

```
ruby postgresql-plugin/test-replication-plugin.rb
```

