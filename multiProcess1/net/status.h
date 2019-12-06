#ifndef TINYWS_STATUS_H
#define TINYWS_STATUS_H

namespace tinyWS_process1 {
    extern int status_quit_softly; //QUIT
    extern int status_terminate;   //TERM,INT
    extern int status_exiting;
    extern int status_restart;
    extern int status_reconfigure; //HUP,reboot
    extern int status_child_quit;  //CHLD
}

#endif //TINYWS_STATUS_H
