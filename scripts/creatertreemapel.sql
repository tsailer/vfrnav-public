CREATE VIRTUAL TABLE mapelements_rtree USING rtree(ID,SWLAT,NELAT,SWLON,NELON);
INSERT INTO mapelements_rtree SELECT ID,SWLAT,NELAT,SWLON,NELON FROM mapelements;
ANALYZE;
VACUUM;