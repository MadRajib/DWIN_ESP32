#include "esp_system.h"
#include "networking/wifi_config.h"
#include "device_config.h"
#include "serial_prompt.h"

/* show current device info */
int show_info(int argc, char **argv)
{
  char tmp[65] = {0};

  printf("Device configs:\n");

  if (!conf_get(CONF_SSID, tmp))
    printf("SSID: %s\n", tmp);
  
  if (!conf_get(CONF_PRINTER_IP, tmp))
    printf("printer_ip: %s:", tmp);
  
  if (!conf_get(CONF_PRINTER_PORT, tmp))
    printf("%s\n:", tmp);

  return 0;
}

int wifi_connect(int argc, char **argv)
{
  if (!is_device_conf_set(CONF_PASS) || !is_device_conf_set(CONF_SSID)) {
    printf("WIFI config not set!\n");
    return -1;
  }

  wifi_init(WIFI_STA);

  return 0;
}

int set_ssid(int argc, char **argv)
{
  if (argc < 2 || argc > 2 ) {
    printf("set_ssid <ssid_name>\n");
    return -1;
  }

  if (conf_set(CONF_SSID, argv[1]))
    printf("error while setting ssid!\n");

  return 0;
}

int set_pass(int argc, char **argv)
{
  if (argc < 2 || argc > 2 ) {
    printf("set_pass <pass_key>");
    return -1;
  }

  if (conf_set(CONF_PASS, argv[1]))
    printf("error while setting pass!\n");

  return 0;
}

int erase(int argc, char **argv)
{
  if (argc < 2) {
    printf("erase ssid/pass/printer_ip/all\n");
    return -1;
  }

  if (!strcmp(argv[1], "all")) {
    conf_erase(CONF_ALL);
  } else if (!strcmp(argv[1], "ssid")) {
    conf_erase(CONF_SSID);
  } else if (!strcmp(argv[1], "pass")) {
    conf_erase(CONF_PASS);
  } else if (!strcmp(argv[1], "printer_ip")) {
    conf_erase(CONF_PRINTER_IP);
    conf_erase(CONF_PRINTER_PORT);
  } else {
    printf("not present!\n");
    return -1;
  }

  return 0;
}

int reboot(int argc, char **argv)
{
  esp_restart();
  return 0;
}

int save_config(int argc, char **argv)
{
  conf_save();
  return 0;
}

int setup_printer_config(int argc, char **argv)
{
 if (argc < 3 || argc > 3 ) {
    printf("set_printer <ip> <port> \n");
    return -1;
  }

  if (conf_set(CONF_PRINTER_IP, argv[1]))
    printf("error while setting printer conf!\n");

  if (conf_set(CONF_PRINTER_PORT, argv[2]))
    printf("error while setting printer port!\n");

  return 0;
}

COMMANDS(
    {"info", "show all info:    -> info", show_info},
    {"set_printer", "set_printer:   -> set_printer <ip> <port>", setup_printer_config},
    {"set_ssid", "set ssid:         -> set_ssid <ssid>", set_ssid},
    {"set_pass", "set pass:         -> set_pass <pass>", set_pass},
    {"erase", "erase config:        -> erase ssid/pass/all", erase},
    {"reboot", "reboot device:      -> reboot", reboot},
    {"save", "save config to device -> save", save_config},
    {"connect", "connect to wifi    -> connect", wifi_connect},
);

void command_init() {
    serial_greet();
}

void commmand_run() {
    serial_run();
}