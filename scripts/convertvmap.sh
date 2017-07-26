#!/bin/sh

echo "Converting VMAP0"
inst/bin/vfrdbmapelementsdb -o . -m gltp:/vrf/usr/local/download/vmap0/v0eur/vmaplv0/eurnasia  -m gltp:/vrf/usr/local/download/vmap0/v0noa/vmaplv0/noamer  -m gltp:/vrf/usr/local/download/vmap0/v0sas/vmaplv0/sasaus  -m gltp:/vrf/usr/local/download/vmap0/v0soa/vmaplv0/soamafr
inst/bin/vfrdbcamelcase mapelements.db
sqlite36 mapelements.db < ~/src/vfrnav/scripts/creatertreemapel.sql
