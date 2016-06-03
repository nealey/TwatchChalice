var initialized = false;

function appMessageAck(e) {
  console.log("Configuration sent");
}

function appMessageNak(e) {
  console.log("Configuration not sent: ", e);
}


Pebble.addEventListener("ready", function() {
  console.log("ready called!");
  initialized = true;
});

Pebble.addEventListener("showConfiguration", function() {
  console.log("showing configuration");
  Pebble.openURL('http://woozle.org/neale/misc/twatch-config/chalice.html');
});

Pebble.addEventListener("webviewclosed", function(e) {
  console.log("configuration closed");
  // webview closed
  var options = JSON.parse(decodeURIComponent(e.response));
  Pebble.sendAppMessage(options, appMessageAck, appMessageNak);
});
