ATTACH DATABASE 'airports1.db' AS db1;

ALTER TABLE airportrunways ADD COLUMN HELAT INTEGER;
ALTER TABLE airportrunways ADD COLUMN HELON INTEGER;
ALTER TABLE airportrunways ADD COLUMN LELAT INTEGER;
ALTER TABLE airportrunways ADD COLUMN LELON INTEGER;
ALTER TABLE airportrunways ADD COLUMN HEDISP INTEGER;
ALTER TABLE airportrunways ADD COLUMN HEHDG INTEGER;
ALTER TABLE airportrunways ADD COLUMN HEELEV INTEGER;
ALTER TABLE airportrunways ADD COLUMN LEDISP INTEGER;
ALTER TABLE airportrunways ADD COLUMN LEHDG INTEGER;
ALTER TABLE airportrunways ADD COLUMN LEELEV INTEGER;
ALTER TABLE airportrunways ADD COLUMN FLAGS INTEGER;
ALTER TABLE airportrunways ADD COLUMN HELIGHT0 INTEGER;
ALTER TABLE airportrunways ADD COLUMN HELIGHT1 INTEGER;
ALTER TABLE airportrunways ADD COLUMN HELIGHT2 INTEGER;
ALTER TABLE airportrunways ADD COLUMN HELIGHT3 INTEGER;
ALTER TABLE airportrunways ADD COLUMN HELIGHT4 INTEGER;
ALTER TABLE airportrunways ADD COLUMN HELIGHT5 INTEGER;
ALTER TABLE airportrunways ADD COLUMN HELIGHT6 INTEGER;
ALTER TABLE airportrunways ADD COLUMN HELIGHT7 INTEGER;
ALTER TABLE airportrunways ADD COLUMN LELIGHT0 INTEGER;
ALTER TABLE airportrunways ADD COLUMN LELIGHT1 INTEGER;
ALTER TABLE airportrunways ADD COLUMN LELIGHT2 INTEGER;
ALTER TABLE airportrunways ADD COLUMN LELIGHT3 INTEGER;
ALTER TABLE airportrunways ADD COLUMN LELIGHT4 INTEGER;
ALTER TABLE airportrunways ADD COLUMN LELIGHT5 INTEGER;
ALTER TABLE airportrunways ADD COLUMN LELIGHT6 INTEGER;
ALTER TABLE airportrunways ADD COLUMN LELIGHT7 INTEGER;

UPDATE main.airportrunways
  SET
    HELAT=(SELECT db1.airportrunways.HELAT
  FROM db1.airportrunways,db1.airports,main.airports
  WHERE
    main.airportrunways.ARPTID=main.airports.ID AND
    main.airports.SRCID=db1.airports.SRCID AND
    db1.airportrunways.ARPTID=db1.airports.ID AND
    main.airportrunways.IDENTHE=db1.airportrunways.IDENTHE AND
    main.airportrunways.IDENTLE=db1.airportrunways.IDENTLE);
UPDATE main.airportrunways
  SET
    HELON=(SELECT db1.airportrunways.HELON
  FROM db1.airportrunways,db1.airports,main.airports
  WHERE
    main.airportrunways.ARPTID=main.airports.ID AND
    main.airports.SRCID=db1.airports.SRCID AND
    db1.airportrunways.ARPTID=db1.airports.ID AND
    main.airportrunways.IDENTHE=db1.airportrunways.IDENTHE AND
    main.airportrunways.IDENTLE=db1.airportrunways.IDENTLE);
UPDATE main.airportrunways
  SET
    LELAT=(SELECT db1.airportrunways.LELAT
  FROM db1.airportrunways,db1.airports,main.airports
  WHERE
    main.airportrunways.ARPTID=main.airports.ID AND
    main.airports.SRCID=db1.airports.SRCID AND
    db1.airportrunways.ARPTID=db1.airports.ID AND
    main.airportrunways.IDENTHE=db1.airportrunways.IDENTHE AND
    main.airportrunways.IDENTLE=db1.airportrunways.IDENTLE);
UPDATE main.airportrunways
  SET
    LELON=(SELECT db1.airportrunways.LELON
  FROM db1.airportrunways,db1.airports,main.airports
  WHERE
    main.airportrunways.ARPTID=main.airports.ID AND
    main.airports.SRCID=db1.airports.SRCID AND
    db1.airportrunways.ARPTID=db1.airports.ID AND
    main.airportrunways.IDENTHE=db1.airportrunways.IDENTHE AND
    main.airportrunways.IDENTLE=db1.airportrunways.IDENTLE);
UPDATE main.airportrunways
  SET
    HEDISP=(SELECT db1.airportrunways.HEDISP
  FROM db1.airportrunways,db1.airports,main.airports
  WHERE
    main.airportrunways.ARPTID=main.airports.ID AND
    main.airports.SRCID=db1.airports.SRCID AND
    db1.airportrunways.ARPTID=db1.airports.ID AND
    main.airportrunways.IDENTHE=db1.airportrunways.IDENTHE AND
    main.airportrunways.IDENTLE=db1.airportrunways.IDENTLE);
UPDATE main.airportrunways
  SET
    HEHDG=(SELECT db1.airportrunways.HEHDG
  FROM db1.airportrunways,db1.airports,main.airports
  WHERE
    main.airportrunways.ARPTID=main.airports.ID AND
    main.airports.SRCID=db1.airports.SRCID AND
    db1.airportrunways.ARPTID=db1.airports.ID AND
    main.airportrunways.IDENTHE=db1.airportrunways.IDENTHE AND
    main.airportrunways.IDENTLE=db1.airportrunways.IDENTLE);
UPDATE main.airportrunways
  SET
    HEELEV=(SELECT db1.airportrunways.HEELEV
  FROM db1.airportrunways,db1.airports,main.airports
  WHERE
    main.airportrunways.ARPTID=main.airports.ID AND
    main.airports.SRCID=db1.airports.SRCID AND
    db1.airportrunways.ARPTID=db1.airports.ID AND
    main.airportrunways.IDENTHE=db1.airportrunways.IDENTHE AND
    main.airportrunways.IDENTLE=db1.airportrunways.IDENTLE);
UPDATE main.airportrunways
  SET
    LEDISP=(SELECT db1.airportrunways.LEDISP
  FROM db1.airportrunways,db1.airports,main.airports
  WHERE
    main.airportrunways.ARPTID=main.airports.ID AND
    main.airports.SRCID=db1.airports.SRCID AND
    db1.airportrunways.ARPTID=db1.airports.ID AND
    main.airportrunways.IDENTHE=db1.airportrunways.IDENTHE AND
    main.airportrunways.IDENTLE=db1.airportrunways.IDENTLE);
UPDATE main.airportrunways
  SET
    LEHDG=(SELECT db1.airportrunways.LEHDG
  FROM db1.airportrunways,db1.airports,main.airports
  WHERE
    main.airportrunways.ARPTID=main.airports.ID AND
    main.airports.SRCID=db1.airports.SRCID AND
    db1.airportrunways.ARPTID=db1.airports.ID AND
    main.airportrunways.IDENTHE=db1.airportrunways.IDENTHE AND
    main.airportrunways.IDENTLE=db1.airportrunways.IDENTLE);
UPDATE main.airportrunways
  SET
    LEELEV=(SELECT db1.airportrunways.LEELEV
  FROM db1.airportrunways,db1.airports,main.airports
  WHERE
    main.airportrunways.ARPTID=main.airports.ID AND
    main.airports.SRCID=db1.airports.SRCID AND
    db1.airportrunways.ARPTID=db1.airports.ID AND
    main.airportrunways.IDENTHE=db1.airportrunways.IDENTHE AND
    main.airportrunways.IDENTLE=db1.airportrunways.IDENTLE);
UPDATE main.airportrunways
  SET
    FLAGS=(SELECT db1.airportrunways.FLAGS
  FROM db1.airportrunways,db1.airports,main.airports
  WHERE
    main.airportrunways.ARPTID=main.airports.ID AND
    main.airports.SRCID=db1.airports.SRCID AND
    db1.airportrunways.ARPTID=db1.airports.ID AND
    main.airportrunways.IDENTHE=db1.airportrunways.IDENTHE AND
    main.airportrunways.IDENTLE=db1.airportrunways.IDENTLE);
UPDATE main.airportrunways
  SET
    HELIGHT0=(SELECT db1.airportrunways.HELIGHT0
  FROM db1.airportrunways,db1.airports,main.airports
  WHERE
    main.airportrunways.ARPTID=main.airports.ID AND
    main.airports.SRCID=db1.airports.SRCID AND
    db1.airportrunways.ARPTID=db1.airports.ID AND
    main.airportrunways.IDENTHE=db1.airportrunways.IDENTHE AND
    main.airportrunways.IDENTLE=db1.airportrunways.IDENTLE);
UPDATE main.airportrunways
  SET
    HELIGHT1=(SELECT db1.airportrunways.HELIGHT1
  FROM db1.airportrunways,db1.airports,main.airports
  WHERE
    main.airportrunways.ARPTID=main.airports.ID AND
    main.airports.SRCID=db1.airports.SRCID AND
    db1.airportrunways.ARPTID=db1.airports.ID AND
    main.airportrunways.IDENTHE=db1.airportrunways.IDENTHE AND
    main.airportrunways.IDENTLE=db1.airportrunways.IDENTLE);
UPDATE main.airportrunways
  SET
    HELIGHT2=(SELECT db1.airportrunways.HELIGHT2
  FROM db1.airportrunways,db1.airports,main.airports
  WHERE
    main.airportrunways.ARPTID=main.airports.ID AND
    main.airports.SRCID=db1.airports.SRCID AND
    db1.airportrunways.ARPTID=db1.airports.ID AND
    main.airportrunways.IDENTHE=db1.airportrunways.IDENTHE AND
    main.airportrunways.IDENTLE=db1.airportrunways.IDENTLE);
UPDATE main.airportrunways
  SET
    HELIGHT3=(SELECT db1.airportrunways.HELIGHT3
  FROM db1.airportrunways,db1.airports,main.airports
  WHERE
    main.airportrunways.ARPTID=main.airports.ID AND
    main.airports.SRCID=db1.airports.SRCID AND
    db1.airportrunways.ARPTID=db1.airports.ID AND
    main.airportrunways.IDENTHE=db1.airportrunways.IDENTHE AND
    main.airportrunways.IDENTLE=db1.airportrunways.IDENTLE);
UPDATE main.airportrunways
  SET
    HELIGHT4=(SELECT db1.airportrunways.HELIGHT4
  FROM db1.airportrunways,db1.airports,main.airports
  WHERE
    main.airportrunways.ARPTID=main.airports.ID AND
    main.airports.SRCID=db1.airports.SRCID AND
    db1.airportrunways.ARPTID=db1.airports.ID AND
    main.airportrunways.IDENTHE=db1.airportrunways.IDENTHE AND
    main.airportrunways.IDENTLE=db1.airportrunways.IDENTLE);
UPDATE main.airportrunways
  SET
    HELIGHT5=(SELECT db1.airportrunways.HELIGHT5
  FROM db1.airportrunways,db1.airports,main.airports
  WHERE
    main.airportrunways.ARPTID=main.airports.ID AND
    main.airports.SRCID=db1.airports.SRCID AND
    db1.airportrunways.ARPTID=db1.airports.ID AND
    main.airportrunways.IDENTHE=db1.airportrunways.IDENTHE AND
    main.airportrunways.IDENTLE=db1.airportrunways.IDENTLE);
UPDATE main.airportrunways
  SET
    HELIGHT6=(SELECT db1.airportrunways.HELIGHT6
  FROM db1.airportrunways,db1.airports,main.airports
  WHERE
    main.airportrunways.ARPTID=main.airports.ID AND
    main.airports.SRCID=db1.airports.SRCID AND
    db1.airportrunways.ARPTID=db1.airports.ID AND
    main.airportrunways.IDENTHE=db1.airportrunways.IDENTHE AND
    main.airportrunways.IDENTLE=db1.airportrunways.IDENTLE);
UPDATE main.airportrunways
  SET
    HELIGHT7=(SELECT db1.airportrunways.HELIGHT7
  FROM db1.airportrunways,db1.airports,main.airports
  WHERE
    main.airportrunways.ARPTID=main.airports.ID AND
    main.airports.SRCID=db1.airports.SRCID AND
    db1.airportrunways.ARPTID=db1.airports.ID AND
    main.airportrunways.IDENTHE=db1.airportrunways.IDENTHE AND
    main.airportrunways.IDENTLE=db1.airportrunways.IDENTLE);
UPDATE main.airportrunways
  SET
    LELIGHT0=(SELECT db1.airportrunways.LELIGHT0
  FROM db1.airportrunways,db1.airports,main.airports
  WHERE
    main.airportrunways.ARPTID=main.airports.ID AND
    main.airports.SRCID=db1.airports.SRCID AND
    db1.airportrunways.ARPTID=db1.airports.ID AND
    main.airportrunways.IDENTHE=db1.airportrunways.IDENTHE AND
    main.airportrunways.IDENTLE=db1.airportrunways.IDENTLE);
UPDATE main.airportrunways
  SET
    LELIGHT1=(SELECT db1.airportrunways.LELIGHT1
  FROM db1.airportrunways,db1.airports,main.airports
  WHERE
    main.airportrunways.ARPTID=main.airports.ID AND
    main.airports.SRCID=db1.airports.SRCID AND
    db1.airportrunways.ARPTID=db1.airports.ID AND
    main.airportrunways.IDENTHE=db1.airportrunways.IDENTHE AND
    main.airportrunways.IDENTLE=db1.airportrunways.IDENTLE);
UPDATE main.airportrunways
  SET
    LELIGHT2=(SELECT db1.airportrunways.LELIGHT2
  FROM db1.airportrunways,db1.airports,main.airports
  WHERE
    main.airportrunways.ARPTID=main.airports.ID AND
    main.airports.SRCID=db1.airports.SRCID AND
    db1.airportrunways.ARPTID=db1.airports.ID AND
    main.airportrunways.IDENTHE=db1.airportrunways.IDENTHE AND
    main.airportrunways.IDENTLE=db1.airportrunways.IDENTLE);
UPDATE main.airportrunways
  SET
    LELIGHT3=(SELECT db1.airportrunways.LELIGHT3
  FROM db1.airportrunways,db1.airports,main.airports
  WHERE
    main.airportrunways.ARPTID=main.airports.ID AND
    main.airports.SRCID=db1.airports.SRCID AND
    db1.airportrunways.ARPTID=db1.airports.ID AND
    main.airportrunways.IDENTHE=db1.airportrunways.IDENTHE AND
    main.airportrunways.IDENTLE=db1.airportrunways.IDENTLE);
UPDATE main.airportrunways
  SET
    LELIGHT4=(SELECT db1.airportrunways.LELIGHT4
  FROM db1.airportrunways,db1.airports,main.airports
  WHERE
    main.airportrunways.ARPTID=main.airports.ID AND
    main.airports.SRCID=db1.airports.SRCID AND
    db1.airportrunways.ARPTID=db1.airports.ID AND
    main.airportrunways.IDENTHE=db1.airportrunways.IDENTHE AND
    main.airportrunways.IDENTLE=db1.airportrunways.IDENTLE);
UPDATE main.airportrunways
  SET
    LELIGHT5=(SELECT db1.airportrunways.LELIGHT5
  FROM db1.airportrunways,db1.airports,main.airports
  WHERE
    main.airportrunways.ARPTID=main.airports.ID AND
    main.airports.SRCID=db1.airports.SRCID AND
    db1.airportrunways.ARPTID=db1.airports.ID AND
    main.airportrunways.IDENTHE=db1.airportrunways.IDENTHE AND
    main.airportrunways.IDENTLE=db1.airportrunways.IDENTLE);
UPDATE main.airportrunways
  SET
    LELIGHT6=(SELECT db1.airportrunways.LELIGHT6
  FROM db1.airportrunways,db1.airports,main.airports
  WHERE
    main.airportrunways.ARPTID=main.airports.ID AND
    main.airports.SRCID=db1.airports.SRCID AND
    db1.airportrunways.ARPTID=db1.airports.ID AND
    main.airportrunways.IDENTHE=db1.airportrunways.IDENTHE AND
    main.airportrunways.IDENTLE=db1.airportrunways.IDENTLE);
UPDATE main.airportrunways
  SET
    LELIGHT7=(SELECT db1.airportrunways.LELIGHT7
  FROM db1.airportrunways,db1.airports,main.airports
  WHERE
    main.airportrunways.ARPTID=main.airports.ID AND
    main.airports.SRCID=db1.airports.SRCID AND
    db1.airportrunways.ARPTID=db1.airports.ID AND
    main.airportrunways.IDENTHE=db1.airportrunways.IDENTHE AND
    main.airportrunways.IDENTLE=db1.airportrunways.IDENTLE);

ANALYZE;
