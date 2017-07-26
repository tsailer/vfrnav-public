select ICAO,count(*) from airports group by ICAO having count(*)>1;
