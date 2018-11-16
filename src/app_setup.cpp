#include "app_setup.h"
#include <FS.h>

SetupMode::SetupMode(const char *clientid, const char *setup_password, ArduinoConfigDB *config)
{
  _clientid = clientid;
  _setup_password = setup_password;
  _config = config;
}

void SetupMode::configRootHandler()
{
    mymap = _config->getMap();
    String output = "";
    output += "<form method='POST' action='/update' />\n";
    output += "<table>\n";
    output += "<tr><th>Key</th><th>Old Value</th><th>New Value</th></tr>\n";
    for (auto kv : mymap) {
        output += "<tr><td>" + kv.first + "</td><td>";
        if (kv.first.endsWith("password")) {
        output += "[hidden]";
        } else {
        output += kv.second;
        }
        output += "</td><td><input type='text' name='v" + kv.first + "' /></td></tr>\n";
    }
    /*
    for (int i=0; i<5; i++) {
        output += "<tr><td><input type='text' name='k' /></td><td></td><td><input type='text' name='v' /></td></tr>\n";
    }
    */
    output += "</table>\n";
    output += "<input type='submit' value='Save' />";
    output += "</form>\n";
    server.send(200, "text/html", output);
}

void SetupMode::configUpdateHandler()
{
  for (int i=0; i<server.args(); i++) {
    if (server.arg(i).length() > 0) {
      Serial.print("updating config ");
      Serial.print(server.argName(i).substring(1));
      Serial.print("=");
      Serial.println(server.arg(i));
      _config->set(server.argName(i).substring(1).c_str(), server.arg(i));
    }
  }
  _config->dump();
  _config->save();
  server.sendHeader("Location", "/");
  server.send(301);
}

void SetupMode::run()
{
  Serial.println("in setup mode now");

  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  WiFi.softAP(_clientid, _setup_password);

  delay(100);
  IPAddress ip = WiFi.softAPIP();

  // display access details
  Serial.print("WiFi AP: SSID=");
  Serial.print(_clientid);
  Serial.print(" URL=http://");
  Serial.print(ip);
  Serial.println("/");

  server.on("/", std::bind(&SetupMode::configRootHandler, this));
  server.on("/update", std::bind(&SetupMode::configUpdateHandler, this));
  server.begin(80);

  while (1) {
    server.handleClient();
  }

  Serial.println("leaving setup mode, restart");
  delay(500);
  ESP.restart();
}
