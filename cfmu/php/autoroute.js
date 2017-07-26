$(function () {
    "use strict";

    // for better performance - to avoid searching in DOM
    var pageparameters = $('#pageparameters');
    var pardepicao = $('#pardepicao');
    var pardepname = $('#pardepname');
    var pardepifr = $('#pardepifr');
    var pardepsidtext = $('#pardepsidtext');
    var pardepsidicao = $('#pardepsidicao');
    var pardepsidname = $('#pardepsidname');
    var pardesticao = $('#pardesticao');
    var pardestname = $('#pardestname');
    var pardestifr = $('#pardestifr');
    var pardeststartext = $('#pardeststartext');
    var pardeststaricao = $('#pardeststaricao');
    var pardeststarname = $('#pardeststarname');
    var parsiddb = $('#parsiddb');
    var parstardb = $('#parstardb');
    var parsidlimit = $('#parsidlimit');
    var parstarlimit = $('#parstarlimit');
    var pardctlimit = $('#pardctlimit');
    var pardctpenalty = $('#pardctpenalty');
    var parbaselevel = $('#parbaselevel');
    var partoplevel = $('#partoplevel');
    var paraircraft = $('#paraircraft');
    var paroptimize = $('#paroptimize');
    var pardate = $('#pardate');
    var pardbawylevels = $('#pardbawylevels');
    var parprecompgraph = $('#parprecompgraph');
    //var parexcluderegions = $('#parexcluderegions');
    var parlocaltfr = $('#parlocaltfr');
    var partracetfr = $('#partracetfr');
    var pardisabletfr = $('#pardisabletfr');
    var parpreflevel = $('#parpreflevel');
    var parprefpenalty = $('#parprefpenalty');
    var parprefclimb = $('#parprefclimb');
    var parprefdescent = $('#parprefdescent');
    var parstart = $('#parstart');
    var pageroute = $('#pageroute');
    var routedep = $('#routedep');
    var routedest = $('#routedest');
    var routeiter = $('#routeiter');
    var routegcdist = $('#routegcdist');
    var routegctime = $('#routegctime');
    var routegcfuel = $('#routegcfuel');
    var routecurdist = $('#routecurdist');
    var routecurtime = $('#routecurtime');
    var routecurfuel = $('#routecurfuel');
    var routecur = $('#routecur');
    var routevalidationresults = $('#routevalidationresults');
    var routemap = $('#routemap');
    var routegraphlog = $('#routegraphlog');
    var routedebuglog = $('#routedebuglog');
    var routeback = $('#routeback');
    var routestop = $('#routestop');
    var pagelog = $('#pagelog');
    var pagecopyright = $('#pagecopyright');

    pageparameters.show();
    pageroute.hide();

    paraircraft.val("P28R Piper Arrow");
    paroptimize.val("Time");

    var sidstarlimit = new Array();
    sidstarlimit[0] = new Array();
    sidstarlimit[1] = new Array();
    sidstarlimit[0][0] = sidstarlimit[1][0] = 20;
    sidstarlimit[0][1] = sidstarlimit[1][1] = 40;
    var fplvalclear = false;
    var depcoord = "";
    var destcoord = "";
    var coordtodeg = 90.0 / 0x40000000;
    var map = null;
    var maproute = null;
    var maproutepts = new Array();
    var swversion = '';

    parstart.prop('disabled', true);
    parsidlimit.val((sidstarlimit[0][+pardepifr.prop('checked')]).toString());
    parstarlimit.val((sidstarlimit[1][+pardestifr.prop('checked')]).toString());

    function checkallparams() {
	var ok = pardepname.html().length >= 4 && pardestname.html().length >= 4 &&
	    depcoord.length > 0 && destcoord.length > 0;
	parstart.prop('disabled', !ok);
    }

    function fmttime(nr) {
	nr = nr | 0;
	var min = (nr / 60) | 0;
	var sec = nr - min * 60;
	var hr = (min / 60) | 0;
	min -= hr * 60;
	sec = sec.toString();
	min = min.toString();
	hr = hr.toString();
	while (sec.length < 2) {
	    sec = '0' + sec;
	}
	while (min.length < 2) {
	    min = '0' + min;
	}
	return hr + ':' + min + ':' + sec;
    }

    function receivemsg(json) {
	if (json === null) {
	    return;
	}
	if (typeof json == "object" && json.constructor === Array) {
	    for (var i = 0; i < json.length; i++) {
		receivemsg(json[i]);
	    }
	    return;
	}
	if (json.hasOwnProperty('error')) {
	    appendlog(json.error);
	}
	if (!json.hasOwnProperty('cmdname')) {
	    if (json.hasOwnProperty('version')) {
		if (swversion == '') {
		    swversion = json.version;
		    if (swversion != '') {
			appendlog("Autorouter Version " + swversion);
		    }
		}
	    }
	    return;
	}
	if (json.cmdname === 'autoroute') {
	    if (json.status === 'stopping') {
		routestop.prop('disabled', true);
	    }
	    return;
	}
	if (json.cmdname === 'log') {
	    if (json.item === 'fplvalidation') {
		if (fplvalclear)
		    routevalidationresults.html('');
		fplvalclear = !json.hasOwnProperty('text') || json.text === '';
		if (!fplvalclear)
		    routevalidationresults.append('<p>' + json.text + '</p>');
		return;
	    }
	    if (json.item === 'graphchange') {
		routegraphlog.append('<p>' + json.text + '</p>');
		return;
	    }
	    if (json.item === 'graphrule') {
		routegraphlog.append('<p>' + json.text + '</p>');
		return;
	    }
	    if (json.item === 'graphruledesc') {
		routegraphlog.append('<p>' + json.text + '</p>');
		return;
	    }
	    if (json.item === 'graphruleoprgoal') {
		routegraphlog.append('<p>' + json.text + '</p>');
		return;
	    }
	    if (json.item.substr(0, 5) === 'debug') {
		routedebuglog.append('<p>' + json.text + '</p>');
		return;
	    }
	    if (json.item === 'normal') {
		appendlog(json.text);
		return;
	    }
	    return;
	}
	if (json.cmdname === 'fpl') {
	    routecur.html('<p>' + json.fpl + '</p>');
	    routeiter.html(json.iteration + ' (' + json.localiteration + '/' + json.remoteiteration + ')');
	    routegcdist.html((+json.gcdist).toFixed(1));
	    routegctime.html(fmttime(json.mintime));
	    routegcfuel.html((+json.minfuel).toFixed(1));
	    routecurdist.html((+json.routedist).toFixed(1));
	    routecurtime.html(fmttime(json.routetime));
	    routecurfuel.html((+json.routefuel).toFixed(1));
	    maproutepts = new Array();
	    for (i = 0; i < json.fplan.length; ++i) {
		maproutepts.push(new google.maps.LatLng(+json.fplan[i].coordlatdeg, +json.fplan[i].coordlondeg));
	    }
	    if (maproute != null) {
		maproute.setMap(null);
		maproute = null;
	    }
	    if (map != null) {
		var opts = {
		    clickable: false,
		    draggable: false,
		    editable: false,
		    geodesic: true,
		    map: map,
		    path: maproutepts,
		    strokeColor: '#FF0000',
		    strokeOpacity: 1.0,
		    strokeWeight: 2
		};
		maproute = new google.maps.Polyline(opts);
	    }
	    return;
	}
	if (json.cmdname === 'departure') {
	    var icao_name = "";
	    if (json.hasOwnProperty('icao')) {
		icao_name = json.icao + '';
	    }
	    if (json.hasOwnProperty('name')) {
		if (icao_name.length > 0 && json.name.length > 0)
		    icao_name += ' ';
		icao_name += json.name;
	    }
	    routedep.html(icao_name);
	    pardepname.html(icao_name);
	    if (json.hasOwnProperty('sidident') && json.hasOwnProperty('sidtype')) {
		pardepsidname.html(json.sidtype + " " + json.sidident);
	    }
	    if (json.hasOwnProperty('coord')) {
		depcoord = json.coord;
	    }
	    if (json.cmdseq !== 'init') {
		checkallparams();
		return;
	    }
	    if (json.hasOwnProperty('sidlimit')) {
		sidstarlimit[0][1] = +json.sidlimit;
		parsidlimit.val((sidstarlimit[0][+pardepifr.prop('checked')]).toString());
	    }
	    if (json.hasOwnProperty('siddb')) {
		parsiddb.prop('checked', json.siddb);
	    }
	    return;
	}
	if (json.cmdname === 'destination') {
	    var icao_name = "";
	    if (json.hasOwnProperty('icao')) {
		icao_name = json.icao + '';
	    }
	    if (json.hasOwnProperty('name')) {
		if (icao_name.length > 0 && json.name.length > 0)
		    icao_name += ' ';
		icao_name += json.name;
	    }
	    routedest.html(icao_name);
	    pardestname.html(icao_name);
	    if (json.hasOwnProperty('starident') && json.hasOwnProperty('startype')) {
		pardeststarname.html(json.startype + " " + json.starident);
	    }
	    if (json.hasOwnProperty('coord')) {
		destcoord = json.coord;
	    }
	    if (json.cmdseq !== 'init') {
		checkallparams()
		return;
	    }
	    if (json.hasOwnProperty('starlimit')) {
		sidstarlimit[1][1] = +json.starlimit;
		parstarlimit.val((sidstarlimit[1][+pardestifr.prop('checked')]).toString());
	    }
	    if (json.hasOwnProperty('stardb')) {
		parstardb.prop('checked', json.stardb);
	    }
	    return;
	}
	if (json.cmdname === 'enroute') {
	    if (json.cmdseq !== 'init') {
		return;
	    }
	    if (json.hasOwnProperty('dctlimit')) {
		pardctlimit.val(json.dctlimit);
	    }
	    if (json.hasOwnProperty('dctpenalty')) {
		pardctpenalty.val(json.dctpenalty);
	    }
	    if (json.hasOwnProperty('honourawylevels')) {
		pardbawylevels.prop('checked', json.honourawylevels);
	    }
	    return;
	}
	if (json.cmdname === 'preferred') {
	    if (json.cmdseq !== 'init') {
		return;
	    }
	    if (json.hasOwnProperty('level')) {
		parpreflevel.val(json.level);
	    }
	    if (json.hasOwnProperty('penalty')) {
		parprefpenalty.val(json.penalty);
	    }
	    if (json.hasOwnProperty('climb')) {
		parprefclimb.val(json.climb);
	    }
	    if (json.hasOwnProperty('descent')) {
		parprefdescent.val(json.descent);
	    }
	    return;
	}
	if (json.cmdname === 'tfr') {
	    if (json.cmdseq !== 'init') {
		return;
	    }
	    if (json.hasOwnProperty('available') && json.available) {
		parlocaltfr.removeProp('disabled');
	    } else {
		parlocaltfr.prop('disabled', true);
	    }
	    if (json.hasOwnProperty('enabled')) {
		parlocaltfr.prop('checked', json.enabled);
	    }
	    if (json.hasOwnProperty('trace')) {
		partracetfr.val(json.trace);
	    }
	    if (json.hasOwnProperty('disable')) {
		pardisabletfr.val(json.disable);
	    }
	    if (json.hasOwnProperty('precompgraph')) {
		parprecompgraph.prop('checked', json.precompgraph);
	    }
	    return;
	}
	if (json.cmdname === 'atmosphere') {
	    if (json.cmdseq !== 'init') {
		return;
	    }
	    if (json.hasOwnProperty('wind')) {
	    }
	    return;
	}
	if (json.cmdname === 'levels') {
	    if (json.cmdseq !== 'init') {
		return;
	    }
	    if (json.hasOwnProperty('base')) {
		parbaselevel.val(json.base);
	    }
	    if (json.hasOwnProperty('top')) {
		partoplevel.val(json.top);
	    }
	    return;
	}
    }

    function datacallback(data,status) {
	if (status == "success") {
	    receivemsg(data);
	}
    }

    function datacallbacklongpoll(data,status) {
	if (status == "success") {
	    receivemsg(data);
	    if (data === null || !data.hasOwnProperty('error')) {
		$.post("io.php",[],datacallbacklongpoll,"json");
	    }
	}
    }

    $.post("io.php?noreply=1",[],function(data,status) {
	if (status == "success") {
	    receivemsg(data);
	    $.post("io.php",[],datacallbacklongpoll,"json");
	}
    });

    function sendmsg(json) {
	$.post("io.php?noreply=1",JSON.stringify(json),datacallback,"json");
    }

    {
	var cmds = new Array();
	cmds.push({cmdname:'departure',cmdseq:'init'});
	cmds.push({cmdname:'destination',cmdseq:'init'});
	cmds.push({cmdname:'enroute',cmdseq:'init'});
	cmds.push({cmdname:'levels',cmdseq:'init'});
	cmds.push({cmdname:'preferred',cmdseq:'init'});
	cmds.push({cmdname:'tfr',cmdseq:'init'});
	cmds.push({cmdname:'atmosphere',cmdseq:'init'});
	cmds.push({cmdname:'preload',cmdseq:'init'});
	sendmsg(cmds);
    }

    function senddeparture() {
	var msg = {cmdname:'departure'};
	var icao = pardepicao.val();
	if (icao.length > 0) {
	    msg.icao = icao;
	}
	var sidicao = pardepsidicao.val();
	if (sidicao.length > 0) {
	    msg.sidident = sidicao;
	}
	var ifr = pardepifr.prop('checked');
	if (ifr) {
	    msg.ifr = true;
	} else {
	    msg.vfr = true;
	}
	msg.sidlimit = +parsidlimit.val();
	msg.siddb = !!parsiddb.prop('checked');
	sendmsg(msg);
    }

    function senddestination() {
	var msg = {cmdname:'destination'};
	var icao = pardesticao.val();
	if (icao.length > 0) {
	    msg.icao = icao;
	}
	var staricao = pardeststaricao.val();
	if (staricao.length > 0) {
	    msg.starident = staricao;
	}
	var ifr = pardestifr.prop('checked');
	if (ifr) {
	    msg.ifr = true;
	} else {
	    msg.vfr = true;
	}
	msg.starlimit = +parstarlimit.val();
	msg.stardb = !!parstardb.prop('checked');
	sendmsg(msg);
    }

    pardepifr.change(function() {
	var ifr = pardepifr.prop('checked');
	parsidlimit.val(sidstarlimit[0][+ifr].toString());
	if (ifr) {
	    pardepsidtext.html('SID End');
	} else {
	    pardepsidtext.html('Joining');
	}
	senddeparture();
    });

    pardestifr.change(function() {
	var ifr = pardestifr.prop('checked');
	parstarlimit.val(sidstarlimit[1][+ifr].toString());
	if (ifr) {
	    pardeststartext.html('STAR Begin');
	} else {
	    pardeststartext.html('Leaving');
	}
	senddestination();
    });

    parsiddb.change(function() {
	senddeparture();
    });

    parstardb.change(function() {
	senddestination();
    });

    parsidlimit.change(function() {
	sidstarlimit[0][+pardepifr.prop('checked')] = +parsidlimit.val();
	senddeparture();
    });

    parstarlimit.change(function() {
	sidstarlimit[1][+pardestifr.prop('checked')] = +parstarlimit.val();
	senddestination();
    });

    pardepifr.change(function() {
	senddeparture();
    });

    pardestifr.change(function() {
	senddestination();
    });

    pardepicao.change(function() {
	senddeparture();
    });

    pardepsidicao.change(function() {
	senddeparture();
    });

    pardesticao.change(function() {
	senddestination();
    });

    pardeststaricao.change(function() {
	senddestination();
    });

    function sendenroute() {
	var msg = {cmdname:'enroute'};
	msg.dctlimit = +pardctlimit.val();
	msg.dctpenalty = +pardctpenalty.val();
	msg.honourawylevels = !!pardbawylevels.prop('checked');
	msg.forceenrouteifr = 1;
	sendmsg(msg);
    }

    pardctlimit.change(function() {
	sendenroute();
    });

    pardctpenalty.change(function() {
	sendenroute();
    });

    pardbawylevels.change(function() {
	sendenroute();
    });

    function sendlevels() {
	var msg = {cmdname:'levels'};
	msg.base = +parbaselevel.val();
	msg.top = +partoplevel.val();
	sendmsg(msg);
    }

    parbaselevel.change(function() {
	sendlevels();
    });

    partoplevel.change(function() {
	sendlevels();
    });

    function sendpreferred() {
	var msg = {cmdname:'preferred'};
	msg.level = +parpreflevel.val();
	msg.penalty = +parprefpenalty.val();
	msg.climb = +parprefclimb.val();
	msg.descent = +parprefdescent.val();
	sendmsg(msg);
    }

    parpreflevel.change(function() {
	sendpreferred();
    });

    parprefpenalty.change(function() {
	sendpreferred();
    });

    parprefclimb.change(function() {
	sendpreferred();
    });

    parprefdescent.change(function() {
	sendpreferred();
    });

    function sendtfr() {
	var msg = {cmdname:'tfr'};
	msg.enabled = !!parlocaltfr.prop('checked');
	msg.trace = partracetfr.val();
	msg.disable = pardisabletfr.val();
	msg.precompgraph = !!parprecompgraph.prop('checked');
	sendmsg(msg);
    }

    parlocaltfr.change(function() {
	sendtfr();
    });

    partracetfr.change(function() {
	sendtfr();
    });

    pardisabletfr.change(function() {
	sendtfr();
    });

    parprecompgraph.change(function() {
	sendtfr();
    });

    function sendaircraft() {
	var msg = {cmdname:'aircraft'};
	var t = paraircraft.val();
	if (t === 'P28R Piper Arrow') {
	    msg.template = 'hbpbx';
	} else if (t === 'P28A Piper Archer') {
	    msg.template = 'hbpho';
	} else if (t === 'M20P Mooney') {
	    msg.template = 'hbdhg';
	} else if (t === 'C172 Cessna 172') {
	    msg.template = 'hbtda';
	} 
	msg.registration = 'ABCDE';
	sendmsg(msg);
    }

    paraircraft.change(function() {
	sendaircraft();
    });

    function sendoptimization() {
	var msg = {cmdname:'optimization'};
	var t = paroptimize.val();
	if (t === 'Time') {
	    msg.target = 'time';
	} else if (t === 'Fuel') {
	    msg.target = 'fuel';
	} else if (t === 'Preferred Level') {
	    msg.target = 'preferred';
	}
	sendmsg(msg);
    }

    paroptimize.change(function() {
	var dis = paroptimize.val() !== 'Preferred Level';
	parpreflevel.prop('disabled', dis);
	parprefpenalty.prop('disabled', dis);
	parprefclimb.prop('disabled', dis);
	parprefdescent.prop('disabled', dis);
	sendoptimization();
    });

    parstart.click(function() {
	pageparameters.hide();
	pageroute.show();
	routestop.prop('disabled', false);
	senddeparture();
	senddestination();
	sendenroute();
	sendlevels();
	sendpreferred();
	sendtfr();
	sendaircraft();
	sendoptimization();
	sendmsg({cmdname:'start'});
	var depc = depcoord.split(",");
	var destc = destcoord.split(",");
	var centerlat = ((+depc[0]) + (+destc[0])) * (0.5 * coordtodeg);
	var centerlon = ((+depc[1]) + (+destc[1])) * (0.5 * coordtodeg);
	var mapoptions = {
            center: new google.maps.LatLng(centerlat, centerlon),
            zoom: 5,
            mapTypeId: google.maps.MapTypeId.ROADMAP
        };
        map = new google.maps.Map(document.getElementById("routemap"), mapoptions);
    });

    routeback.click(function() {
	pageparameters.show();
	pageroute.hide();
	sendmsg({cmdname:'stop'});
    });

    routestop.click(function() {
	sendmsg({cmdname:'stop'});
	routestop.prop('disabled', true);
    });

    /**
     * Add to the error log
     */
    function appendlog(msg) {
	pagelog.append('<p>' + msg + '</p>');
    }
});
