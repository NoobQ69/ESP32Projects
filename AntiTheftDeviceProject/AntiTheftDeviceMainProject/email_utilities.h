#ifndef EMAIL_UTILITIES_H
#define EMAIL_UTILITIES_H

// name: Cartos Los
// email name: loscartos6@gmail.com
// pass: lc10936@#%pass
// xjbrqswgamqafmoc
// Server: 192.168.1.200

/** The smtp host name e.g. smtp.gmail.com for GMail or smtp.office365.com for Outlook or smtp.mail.yahoo.com */
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465

/* The sign in credentials */
#define AUTHOR_EMAIL "loscartos6@gmail.com"
#define AUTHOR_PASSWORD "xjbrqswgamqafmoc"

/* Recipient's email*/
#define RECIPIENT_EMAIL "enter001vn@gmail.com"

typedef struct
{
  String author_email;
  String author_pass;
  String recipient_email;

} email_data_t;

/* Declare the global used SMTPSession object for SMTP transport */
SMTPSession smtp;
SMTP_Message message;
Session_Config config;

/* Callback function to get the Email sending status */

void smtp_callback(SMTP_Status status)
{
  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success())
  {
    // ESP_MAIL_PRINTF used in the examples is for format printing via debug Serial port
    // that works for all supported Arduino platform SDKs e.g. AVR, SAMD, ESP32 and ESP8266.
    // In ESP8266 and ESP32, you can use Serial.printf directly.

    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failed: %d\n", status.failedCount());
    Serial.println("----------------\n");

    for (size_t i = 0; i < smtp.sendingResult.size(); i++)
    {
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);

      // In case, ESP32, ESP8266 and SAMD device, the timestamp get from result.timestamp should be valid if
      // your device time was synched with NTP server.
      // Other devices may show invalid timestamp as the device time was not set i.e. it will show Jan 1, 1970.
      // You can call smtp.setSystemTime(xxx) to set device time manually. Where xxx is timestamp (seconds since Jan 1, 1970)
      
      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %s\n", MailClient.Time.getDateTimeString(result.timestamp, "%B %d, %Y %H:%M:%S").c_str());
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
    }
    Serial.println("----------------\n");

    // You need to clear sending result as the memory usage will grow up.
    smtp.sendingResult.clear();
  }
}

void set_section_config(Session_Config &config_var, String author_name, String author_pass)
{
  /* Set the session config */
  config_var.server.host_name = SMTP_HOST;
  config_var.server.port = SMTP_PORT;
  if (author_name = "")
  {
    config_var.login.email = AUTHOR_EMAIL;
  }
  else
  {
    config_var.login.email = author_name.c_str();
  }
  if (author_pass = "")
  {
    config_var.login.password = AUTHOR_PASSWORD;
  }
  else
  {
    config_var.login.password = author_pass;
  }
  config_var.login.user_domain = "";

  config_var.time.ntp_server = F("pool.ntp.org,time.nist.gov");
  config_var.time.gmt_offset = 3;
  config_var.time.day_light_offset = 0;
}

bool set_stmp_message(SMTP_Message &message_var, String author_email, String recipient_email)
{
    /* Set the message headers */
  if (author_email == "") 
    return false;
  if (recipient_email == "") 
    return false;

  message_var.sender.name = F("ESP-antitheft-device");
  message_var.sender.email = author_email.c_str();
  message_var.subject = F("ESP Device Email");
  message_var.addRecipient(F("Sara"), recipient_email.c_str());

  //Send raw text message
  String textMsg = "Alert! - Someone's trying to open the door!";
  message_var.text.content = textMsg.c_str();
  message_var.text.charSet = "us-ascii";
  message_var.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
  
  message_var.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;
  message_var.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;
  
  return true;
}

bool smtp_init(email_data_t email_data)
{
  /*  Set the network reconnection option */
  MailClient.networkReconnect(true);

  /** Enable the debug via Serial port
   * 0 for no debugging
   * 1 for basic level debugging
   *
   * Debug port can be changed via ESP_MAIL_DEFAULT_DEBUG_PORT in ESP_Mail_FS.h
   */
  smtp.debug(1);

  /* Set the callback function to get the sending results */
  smtp.callback(smtp_callback);

  /* Declare the Session_Config for user defined session credentials */
  set_section_config(config, email_data.author_email, email_data.author_pass);

  /* Declare the message class */

  if (!set_stmp_message(message, email_data.author_email, email_data.recipient_email))
  {
    return false;
  }

  /* Connect to the server */
  if (!smtp.connect(&config))
  {
    ESP_MAIL_PRINTF("Connection error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
    return false;
  }

  if (!smtp.isLoggedIn())
  {
    Serial.println("\nNot yet logged in.");
  }
  else
  {
    if (smtp.isAuthenticated())
      Serial.println("\nSuccessfully logged in.");
    else
      Serial.println("\nConnected with no Auth.");
  }

  return true;
}

bool smtp_send_email(SMTPSession &smtp_email, SMTP_Message &message_email)
{
  /* Start sending Email and close the session */
  if (!MailClient.sendMail(&smtp_email, &message_email))
  {
    ESP_MAIL_PRINTF("Error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
    return false;
  }
  return true;
}

#endif
