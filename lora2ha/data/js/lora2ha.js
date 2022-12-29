var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
var client;

function onLoad(event) {
	initWebSocket();
	$('#formconf').submit(function( event ) {
		event.preventDefault();
		var posting = $.post(event.currentTarget.action, $(this).serialize() );
		posting.done(function( data ) {
			$("#resultconf").fadeTo(100, 1);
			window.setTimeout(function() {$("#resultconf").fadeTo(500, 0)}, 2000);
			//console.log(data);
		});
	});
	$('#formwifi').submit(function( event ) {
		event.preventDefault();
		var posting = $.post(event.currentTarget.action, $(this).serialize() );
		posting.done(function( data ) {
			$("#resultwifi").fadeTo(100, 1);
			window.setTimeout(function() {$("#resultwifi").fadeTo(500, 0)}, 2000);
			//console.log(data);
		});
	});
}

function initWebSocket() {
	//console.log('Trying to open a WebSocket connection...');
	websocket = new WebSocket(gateway);
	websocket.onopen    = WSonOpen;
	websocket.onclose   = WSonClose;
	websocket.onmessage = WSonMessage;
}

function WSonOpen(event) {
	//console.log('Connection opened');
}

function WSonClose(event) {
	//console.log('Connection closed');
}

function WSonMessage(event) {
	var cmd = "";
	console.log(event.data);
	JSON.parse(event.data, (key, value) => {
		//console.log(cmd + ":" + key+'='+value);
		if (key === "cmd") {
			cmd = value;
		} else
		if (key != "" && typeof value !== 'object') {
			if (cmd === "html") {
				$(key).html( value );
			} else
			if (cmd === "replace") {
				$(key).replaceWith( value );
			}
		} else {
			if (cmd === "loadselect") {
				loadSelect();
			}
		}
	});
}

function loadSelect() {
}

function adddev(elt) {
	var nbdev = $("input[name$='address']").length;
	$("#conf_dev").append("<div id='newdev_"+nbdev+"'>device "+nbdev+"...</div>");
	websocket.send("adddev;"+nbdev);
}
function addchild(elt) {
	var arr = elt.id.split(/[_]+/);
	var nbchild = $("#develt_"+arr[1]).children('div').length - 2;
	$("<div id='conf_child_"+arr[1]+"_"+nbchild+"' class='flex-wrap'>chargement...</div>").insertAfter("#conf_child_"+arr[1]+"_"+(nbchild-1));
	websocket.send("addchild;"+arr[1]+";"+nbchild);
}
