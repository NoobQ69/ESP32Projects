// Google script Web_App_URL.
String Web_App_URL = "REPLACE_WITH_YOUR_WEB_APP_URL";

String byteArrayToString(byte array[], unsigned int length) {
  String stringUID = "";
  for (unsigned int i = 0; i < length; i++)
  {
    byte nib1 = (array[i] >> 4) & 0x0F;
    byte nib2 = (array[i] >> 0) & 0x0F;
    stringUID += String(nib1 < 0xA ? '0' + nib1 : 'A' + nib1 - 0xA);
    stringUID += String(nib2 < 0xA ? '0' + nib2 : 'A' + nib2 - 0xA);
  }
  return stringUID;
}

String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;
  
  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

bool httpRequestToScriptApp(String strModes, String strUID) {
  if (WiFi.status() == WL_CONNECTED) {
    String httpRequestUrl = "";

    //----------------------------------------Create links to make HTTP requests to Google Sheets.
    if (strModes == "atc") {
      httpRequestUrl  = Web_App_URL + "?sts=atc";
      httpRequestUrl += "&uid=" + strUID;
    }
    if (strModes == "reg") {
      httpRequestUrl = Web_App_URL + "?sts=reg";
      httpRequestUrl += "&uid=" + strUID;
    }
    //----------------------------------------
#ifdef DEBUG_ON
    //----------------------------------------Sending HTTP requests to Google Sheets.
    Serial.println();
    Serial.println("-------------");
    Serial.println("Sending request to Google Sheets...");
    Serial.print("URL : ");
    Serial.println(httpRequestUrl);
#endif
    // Create an HTTPClient object as "http".
    HTTPClient http;

    // HTTP GET Request.
    http.begin(httpRequestUrl.c_str());
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    // Gets the HTTP status code.
    int httpCode = http.GET(); 
    Serial.print("HTTP Status Code : ");
    Serial.println(httpCode);

    // Getting response from google sheet.
    String payload;
    if (httpCode > 0) {
        payload = http.getString();
#ifdef DEBUG_ON
        Serial.println("Payload : " + payload);  
        Serial.println("-------------");
#endif
    }
    http.end();

    String sts_Res = getValue(payload, ',', 0);
    //----------------------------------------Conditions that are executed are based on the payload response from Google Sheets (the payload response is set in Google Apps Script).
    if (sts_Res == "OK") {
      //..................
      if (strModes == "atc") {
        atcInfo = getValue(payload, ',', 1);
        
        if (atcInfo == "TI_Successful") {

        }

        if (atcInfo == "TO_Successful") {

        }

        if (atcInfo == "atcInf01") {
            // dosomething else
        }

        if (atcInfo == "atcErr01") {
            //handle error
        }

        if (atcInfo == "atcErr02") {
            //handle error
        }
      }
      //..................

      //..................
      if (strModes == "reg") {
        regInfo = getValue(payload, ',', 1);
        
        if (regInfo == "R_Successful") {
            // do something else
        }

        if (regInfo == "regErr01") {
            // handle error
        }

        regInfo = "";
      }
    }
    } 
    return false;
}