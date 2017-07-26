CREATE VIRTUAL TABLE airspaces_rtree USING rtree(ID,SWLAT,NELAT,SWLON,NELON);
INSERT INTO airspaces_rtree SELECT ID,SWLAT,NELAT,SWLON,NELON FROM airspaces;
ANALYZE;
VACUUM;