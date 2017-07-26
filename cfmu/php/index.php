<?php include('auth.php'); ?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="de" lang="de">
  <head>
    <meta charset="utf-8">
    <title>CFMU IFR Autorouter</title>
    <style>
      * { font-family:tahoma; font-size:12px; padding:0px; margin:0px; }
      p { line-height:18px; }
      div { width:800px; margin-left:auto; margin-right:auto;}
      h1 { font-size:18px; line-height:27px; }
      #routecur { padding:5px; border-radius:5px; overflow-y: scroll; border:1px margin-top:10px; height: 80px; }
      #routevalidationresults { padding:5px; border-radius:5px; overflow-y: scroll; border:1px margin-top:10px; height: 80px; }
      #routemap { padding:5px; border-radius:5px; overflow-y: scroll; border:1px margin-top:10px; height: 480px; }
      #routegraphlog { padding:5px; border-radius:5px; overflow-y: scroll; border:1px margin-top:10px; height: 160px; }
      #routedebuglog { padding:5px; border-radius:5px; overflow-y: scroll; border:1px margin-top:10px; height: 160px; }
      #pagelog { padding:5px; border-radius:5px; overflow-y: scroll; border:1px margin-top:10px; height: 160px; }
    </style>
  </head>
  <body>
    <div id="pageparameters">
      <h1>CFMU Autorouter Parameters</h1>
      <table>
        <tbody>
          <tr>
            <td>Departure</td>
            <td><input type="text" id="pardepicao" pattern="[0-9A-Za-z]*" maxlength=4 size=4 /></td>
            <td id="pardepname" width="200"></td>
            <td><input type="checkbox" id="pardepifr" checked>IFR</input></td>
            <td id="pardepsidtext">SID End</td>
            <td><input type="text" id="pardepsidicao" pattern="[0-9A-Za-z]*" maxlength=5 size=5 /></td>
            <td id="pardepsidname" width="200"></td>
            <td><input type="checkbox" id="parsiddb">Use SID Database</input></td>
          </tr>
          <tr>
            <td>Destination</td>
            <td><input type="text" id="pardesticao" pattern="[0-9A-Za-z]*" maxlength=4 size=4 /></td>
            <td id="pardestname" width="200"></td>
            <td><input type="checkbox" id="pardestifr" checked>IFR</input></td>
            <td id="pardeststartext">STAR Begin</td>
            <td><input type="text" id="pardeststaricao" pattern="[0-9A-Za-z]*" maxlength=5 size=5 /></td>
            <td id="pardeststarname" width="200"></td>
            <td><input type="checkbox" id="parstardb">Use STAR Database</input></td>
          </tr>
        </tbody>
      </table>
      <table>
        <tbody>
          <tr>
            <td>SID Limit:</td>
            <td><input type="number" id="parsidlimit" pattern="[0-9]*" maxlength=4 size=4 min=0 max=100 step=1 value=40 /></td>
            <td>NM</td>
            <td>STAR Limit:</td>
            <td><input type="number" id="parstarlimit" pattern="[0-9]*" maxlength=4 size=4 min=0 max=100 step=1 value=40 /></td>
            <td>NM</td>
          </tr>
          <tr>
            <td>DCT Limit:</td>
            <td><input type="number" id="pardctlimit" pattern="[0-9]*" maxlength=4 size=4 min=0 max=100 step=1 value=50 /></td>
            <td>NM</td>
            <td>DCT Penalty:</td>
            <td><input type="number" id="pardctpenalty" pattern="[0-9\.]*" maxlength=7 size=7 min=0.001 max=1000 step=1 value=1.000 /></td>
            <td>ratio</td>
          </tr>
          <tr>
            <td>Base Level:</td>
            <td><input type="number" id="parbaselevel" pattern="[0-9]*" maxlength=4 size=4 min=0 max=600 step=10 value=50 /></td>
            <td>Hectofeet</td>
            <td>Top Level:</td>
            <td><input type="number" id="partoplevel" pattern="[0-9]*" maxlength=4 size=4 min=0 max=600 step=10 value=120 /></td>
            <td>Hectofeet</td>
          </tr>
          <tr>
            <td>Aircraft:</td>
            <td><select id="paraircraft" size="4" >
              <option>P28A Piper Archer</option>
              <option>P28R Piper Arrow</option>
              <option>M20P Mooney</option>
              <option>C172 Cessna 172</option>
            </select></td>
            <td></td>
            <td>Optimize:</td>
            <td><select id="paroptimize" size="4" >
              <option>Time</option>
              <option>Fuel</option>
              <option>Preferred Level</option>
            </select></td>
            <td></td>
          <!--</tr>
            <td>date</td>
            <td><input type="datetime" id="pardate" /></td>
          <tr>
          -->
          </tr>
        </tbody>
      </table>
      <table>
        <tbody>
          <tr>
            <td><input type="checkbox" id="pardbawylevels" checked>Use Database Airway Levels</input></td>
            <td><input type="checkbox" id="parprecompgraph" checked>Use Precomputed Graph</input></td>
            <td><!--Exclude Regions:--></td>
            <td><!--<input type="text" id="parexcluderegions" pattern="[0-9A-Za-z]*" size=50 />--></td>
          </tr>
        </tbody>
      </table>
      <table>
        <tbody>
          <tr>
            <td><input type="checkbox" id="parlocaltfr" checked>Local Traffic Flow Restrictions</input></td>
            <td>Trace TFR:</td>
            <td><input type="text" id="partracetfr" pattern="[0-9A-Za-z ]*" size=20 /></td>
            <td>Disable TFR:</td>
            <td><input type="text" id="pardisabletfr" pattern="[0-9A-Za-z ]*" size=20 /></td>
          </tr>
        </tbody>
      </table>
      <table>
        <tbody>
          <tr>
            <td>Preferred Level:</td>
            <td><input type="number" id="parpreflevel" pattern="[0-9]*" maxlength=4 size=4 min=0 max=600 step=10 value=100 disabled /></td>
            <td>Penalty:</td>
            <td><input type="number" id="parprefpenalty" pattern="[0-9\.]*" maxlength=6 size=6 min=1 max=10 step=0.01 value=1.100 disabled /></td>
            <td>Climb:</td>
            <td><input type="number" id="parprefclimb" pattern="[0-9\.]*" maxlength=7 size=7 min=0 max=100 step=0.1 value=10 disabled /></td>
            <td>Descent:</td>
            <td><input type="number" id="parprefdescent" pattern="[0-9\.]*" maxlength=7 size=7 min=0 max=100 step=0.1 value=3 disabled /></td>
          </tr>
        </tbody>
      </table>
      <input type="submit" id="parstart" value="Start"/>
    </div>
    <div id="pageroute">
      <h1>CFMU Autorouter Routing</h1>
      <table>
        <tbody>
          <tr>
            <td>Departure</td>
            <td id="routedep" colspan=4 ></td>
            <td>Iteration</td>
          </tr>
          <tr>
            <td>Destination</td>
            <td id="routedest" colspan=4 ></td>
            <td id="routeiter" >0 (0/0)</td>
          </tr>
          <tr>
            <td>GC Distance</td>
            <td id="routegcdist"></td>
            <td>NM Time</td>
            <td id="routegctime"></td>
            <td>Fuel</td>
            <td id="routegcfuel"></td>
          </tr>
          <tr>
            <td>Route Distance</td>
            <td id="routecurdist"></td>
            <td>NM Time</td>
            <td id="routecurtime"></td>
            <td>Fuel</td>
            <td id="routecurfuel"></td>
          </tr>
        </tbody>
      </table>
      <p>Current Route:</p>
      <div id="routecur"></div>
      <p>Validation Results:</p>
      <div id="routevalidationresults"></div>
      <div id="routemap"></div>
      <p>Graph Log:</p>
      <div id="routegraphlog"></div>
      <p>Debug Log:</p>
      <div id="routedebuglog"></div>
      <a href="https://www.public.cfmu.eurocontrol.int/PUBPORTAL/gateway/spec/index.html">CFMU NOP Portal</a>
      <table>
        <tbody>
          <tr>
            <td><input type="button" id="routeback" value="Back"/></td>
            <td><input type="button" id="routestop" value="Stop"/></td>
          </tr>
        </tbody>
      </table>
    </div>
    <hr>
    <h1>Log</h1>
    <div id="pagelog"></div>
    <hr>
    <div id="pagecopyright">
      <p>Copyright &copy; Thomas Sailer - see <A href="http://www.baycom.org/~tom/vfrnav" name="vfrnav_project">vfrnav</A> - You can <a href="logout.php">log out here</a>.</p>
    </div>
    <script src="http://ajax.googleapis.com/ajax/libs/jquery/1.9.1/jquery.min.js"></script>
    <script src="http://crypto-js.googlecode.com/svn/tags/3.1.2/build/rollups/sha256.js"></script>
    <script src="http://maps.googleapis.com/maps/api/js?v=3.exp&sensor=false"></script>
    <script src="autoroute.js"></script>
  </body>
</html>
