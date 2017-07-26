$(function () {
    "use strict";

    // for better performance - to avoid searching in DOM
    var pagelogin = $('#pagelogin');
    var loginusername = $('#loginusername');
    var loginpassword = $('#loginpassword');
    var loginsubmit = $('#loginsubmit');
    var loginmessage = $('#loginmessage');
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

    function loginsubmitenadis() {
	loginsubmit.prop("disabled", !(loginusername.val().length > 0 && loginpassword.val().length > 0));
    }

    pagelogin.show();
    loginsubmitenadis();
    loginusername.prop("disabled", true);
    loginpassword.prop("disabled", true);
    loginmessage.html("");
    pageparameters.hide();
    pageroute.hide();

    paraircraft.val("P28R Piper Arrow");
    paroptimize.val("Time");
    parstart.prop('disabled', true);

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

    // if user is running mozilla then use it's built-in WebSocket
    window.WebSocket = window.WebSocket || window.MozWebSocket;

    // if browser doesn't support WebSocket, just show some notification and exit
    if (!window.WebSocket) {
	appendlog('Sorry, but your browser doesn\'t support WebSockets.');
	pagelogin.hide();
	return;
    }

    // open connection
    var connection = new WebSocket('ws://127.0.0.1:9002');

    connection.onopen = function () {
        loginusername.removeProp('disabled');
        loginpassword.removeProp('disabled');
        appendlog('Please Login.');
    };

    connection.onerror = function (error) {
        // just in there were some problems with conenction...
        appendlog('Sorry, but there\'s some problem with your connection or the server is down.');
    };

    // most important part - incoming messages
    connection.onmessage = function (message) {
        // try to parse JSON message. Because we know that the server always returns
        // JSON this should work without any problem but we should make sure that
        // the massage is not chunked or otherwise damaged.
        try {
            var json = JSON.parse(message.data);
        } catch (e) {
            appendlog('This doesn\'t look like a valid JSON: ' + message.data);
            return;
        }
	if (json.hasOwnProperty('error')) {
	    appendlog(json.error);
	}
	if (json.cmd === 'loginchallenge') {
	    if (json.challenge === '') {
		loginmessage.html("<p>Server Error</p>");
		loginsubmit.prop("disabled", true);
		loginusername.prop("disabled", false);
		loginpassword.prop("disabled", false);
		loginpassword.val("");
		return;
	    }
	    var msg = {};
	    msg['cmd'] = 'login';
	    msg['user'] = loginusername.val();
	    var seq = (new Date().getTime()).toString(16);
	    msg['seq'] = seq;
	    var hash = CryptoJS.SHA256(loginpassword.val() + json.challenge + seq);
	    msg['hash'] = hash.toString(CryptoJS.enc.Hex);
	    connection.send(JSON.stringify(msg));
	    return;
	}
	if (json.cmd === 'loginresult') {
	    if (json.result) {
		pagelogin.hide();
		pageparameters.show();
		parstart.prop('disabled', true);
		parsidlimit.val(sidstarlimit[0][+pardepifr.prop('checked')].toString());
		parstarlimit.val(sidstarlimit[1][+pardestifr.prop('checked')].toString());
		var msg = {};
		msg['cmdseq'] = 'init';
		msg['cmd'] = 'departure';
		connection.send(JSON.stringify(msg));
		msg['cmd'] = 'destination';
		connection.send(JSON.stringify(msg));
		msg['cmd'] = 'enroute';
		connection.send(JSON.stringify(msg));
		msg['cmd'] = 'levels';
		connection.send(JSON.stringify(msg));
		msg['cmd'] = 'preferred';
		connection.send(JSON.stringify(msg));
		msg['cmd'] = 'tfr';
		connection.send(JSON.stringify(msg));
		msg['cmd'] = 'atmosphere';
		connection.send(JSON.stringify(msg));
		msg['cmd'] = 'preload';
		connection.send(JSON.stringify(msg));
	    } else {
		loginsubmit.prop("disabled", true);
		loginusername.prop("disabled", false);
		loginpassword.prop("disabled", false);
		loginpassword.val("");
	    }
	    return;
 	}
	if (json.cmd === 'autoroute') {
	    if (json.status === 'stopping') {
		routestop.prop('disabled', true);
	    }
	    return;
	}
	if (json.cmd === 'log') {
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
	if (json.cmd === 'fplbegin') {
	    maproutepts = new Array();
	    return;
	}
	if (json.cmd === 'fplwpt') {
	    if (json.hasOwnProperty('coord')) {
		var c = (json.coord + '').split(',');
		maproutepts.push(new google.maps.LatLng((+c[0]) * coordtodeg, (+c[1]) * coordtodeg));
	    }
	    return;
	}
	if (json.cmd === 'fplend') {
	    routecur.html('<p>' + json.fpl + '</p>');
	    routeiter.html(json.iteration + ' (' + json.localiteration + '/' + json.remoteiteration + ')');
	    routegcdist.html((+json.gcdist).toFixed(1));
	    routegctime.html(fmttime(json.mintime));
	    routegcfuel.html((+json.minfuel).toFixed(1));
	    routecurdist.html((+json.routedist).toFixed(1));
	    routecurtime.html(fmttime(json.routetime));
	    routecurfuel.html((+json.routefuel).toFixed(1));
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
	if (json.cmd === 'departure') {
	    var icao_name = json.icao + '';
	    if (icao_name.length > 0 && json.name.length > 0)
		icao_name += ' ';
	    icao_name += json.name;
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
		parsidlimit.val(sidstarlimit[0][pardepifr.prop('checked')]);
	    }
	    return;
	}
	if (json.cmd === 'destination') {
	    var icao_name = json.icao + '';
	    if (icao_name.length > 0 && json.name.length > 0)
		icao_name += ' ';
	    icao_name += json.name;
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
		parstarlimit.val(sidstarlimit[1][pardestifr.prop('checked')]);
	    }
	    return;
	}
	if (json.cmd === 'enroute') {
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
	if (json.cmd === 'preferred') {
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
	if (json.cmd === 'tfr') {
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
	    return;
	}
	if (json.cmd === 'atmosphere') {
	    if (json.cmdseq !== 'init') {
		return;
	    }
	    if (json.hasOwnProperty('wind')) {
	    }
	    return;
	}
	if (json.cmd === 'levels') {
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

        // NOTE: if you're not sure about the JSON structure
        // check the server source code above
        // if (json.type === 'color') { // first response from the server with user's color
        //     myColor = json.data;
        //     status.text(myName + ': ').css('color', myColor);
        //     input.removeProp('disabled').focus();
        //     // from now user can start sending messages
        // } else if (json.type === 'history') { // entire message history
        //     // insert every single message to the chat window
        //     for (var i=0; i < json.data.length; i++) {
        //         addMessage(json.data[i].author, json.data[i].text,
        //                    json.data[i].color, new Date(json.data[i].time));
        //     }
        // } else if (json.type === 'message') { // it's a single message
        //     input.removeProp('disabled'); // let the user write another message
        //     addMessage(json.data.author, json.data.text,
        //                json.data.color, new Date(json.data.time));
        // } else {
        //     console.log('Hmm..., I\'ve never seen JSON like this: ', json);
        // }
    };

    loginusername.change(function() {
	loginsubmitenadis();
    });
    
    loginpassword.change(function() {
	loginsubmitenadis();
    });

    loginsubmit.click(function() {
	loginsubmit.prop("disabled", true);
	loginusername.prop("disabled", true);
	loginpassword.prop("disabled", true);
	var msg = {};
	msg['cmd'] = 'loginstart';
	connection.send(JSON.stringify(msg));
    });

    function senddeparture() {
	var msg = {};
	msg.cmd = 'departure';
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
	    msg.ifr = '';
	} else {
	    msg.vfr = '';
	}
	msg.sidlimit = parsidlimit.val();
	connection.send(JSON.stringify(msg));
    }

    function senddestination() {
	var msg = {};
	msg.cmd = 'destination';
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
	    msg.ifr = '';
	} else {
	    msg.vfr = '';
	}
	msg.starlimit = parstarlimit.val();
	connection.send(JSON.stringify(msg));
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
	var msg = {};
	msg.cmd = 'enroute';
	msg.dctlimit = pardctlimit.val();
	msg.dctpenalty = pardctpenalty.val();
	msg.honourawylevels = +pardbawylevels.prop('checked');
	msg.forceenrouteifr = 1;
	connection.send(JSON.stringify(msg));
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
	var msg = {};
	msg.cmd = 'levels';
	msg.base = parbaselevel.val();
	msg.top = partoplevel.val();
	connection.send(JSON.stringify(msg));
    }

    parbaselevel.change(function() {
	sendlevels();
    });

    partoplevel.change(function() {
	sendlevels();
    });

    function sendpreferred() {
	var msg = {};
	msg.cmd = 'preferred';
	msg.level = parpreflevel.val();
	msg.penalty = parprefpenalty.val();
	msg.climb = parprefclimb.val();
	msg.descent = parprefdescent.val();
	connection.send(JSON.stringify(msg));
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
	var msg = {};
	msg.cmd = 'tfr';
	msg.enabled = +parlocaltfr.prop('checked');
	msg.trace = partracetfr.val();
	msg.disable = pardisabletfr.val();
	connection.send(JSON.stringify(msg));
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

    function sendaircraft() {
	var msg = {};
	msg.cmd = 'aircraft';
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
	connection.send(JSON.stringify(msg));
    }

    paraircraft.change(function() {
	sendaircraft();
    });

    function sendoptimization() {
	var msg = {};
	msg.cmd = 'optimization';
	var t = paroptimize.val();
	if (t === 'Time') {
	    msg.target = 'time';
	} else if (t === 'Fuel') {
	    msg.target = 'fuel';
	} else if (t === 'Preferred Level') {
	    msg.target = 'preferred';
	}
	connection.send(JSON.stringify(msg));
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
	var msg = {};
	msg.cmd = 'start';
	connection.send(JSON.stringify(msg));
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
	var msg = {};
	msg.cmd = 'stop';
	connection.send(JSON.stringify(msg));
    });

    routestop.click(function() {
	var msg = {};
	msg.cmd = 'stop';
	connection.send(JSON.stringify(msg));
	routestop.prop('disabled', true);
    });

    /**
     * Add to the error log
     */
    function appendlog(msg) {
	pagelog.append('<p>' + msg + '</p>');
    }
});
