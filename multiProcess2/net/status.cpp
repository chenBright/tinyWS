#include "status.h"

int tinyWS_process::status_quit_softly = 0; //QUIT
int tinyWS_process::status_terminate = 0;   //TERM,INT
int tinyWS_process::status_exiting = 0;
int tinyWS_process::status_restart = 0;
int tinyWS_process::status_reconfigure = 0; //HUP,reboot
int tinyWS_process::status_child_quit = 0;  //CHLD
