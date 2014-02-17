//
//  openiapp.c
//  openiapp
//
//  Created by cal0x on 15.01.14.
//  Copyright 2014. All rights reserved
//
//

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <libgen.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>

#include <zlib.h>

#include <fcntl.h>
#include <sys/mman.h>

#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include <libimobiledevice/mobile_image_mounter.h>
#include <libimobiledevice/mobilebackup2.h>
#include <libimobiledevice/installation_proxy.h>
#include <libimobiledevice/notification_proxy.h>
#include <libimobiledevice/afc.h>
#include <libimobiledevice/sbservices.h>
#include <libimobiledevice/file_relay.h>
#include <libimobiledevice/diagnostics_relay.h>

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

uint64_t gFh = 0;
unsigned int cb = 0;
unsigned int installError = 0;
unsigned int installing = 1;

idevice_t idevice = NULL;
afc_client_t afc_client = NULL;
lockdownd_client_t lockdownd_client = NULL;
instproxy_client_t gInstproxy = NULL;

int afc_send_file(afc_client_t afc, const char* local, const char* remote) {
	FILE* fd = NULL;
	uint64_t fh = 0;
	afc_error_t err = 0;
	unsigned int got = 0;
	unsigned int gave = 0;
	unsigned char buffer[0x800];
    
	fd = fopen(local, "rb");
	if (fd != NULL ) {
		err = afc_file_open(afc, remote, AFC_FOPEN_WR, &fh);
		if (err == AFC_E_SUCCESS) {
            
			while (!feof(fd)) {
				memset(buffer, '\0', sizeof(buffer));
				got = fread(buffer, 1, sizeof(buffer), fd);
				if (got > 0) {
					afc_file_write(afc, fh, (const char*) buffer, got, &gave);
					if (gave != got) {
						printf("Error!!\n");
						break;
					}
				}
			}
            
			afc_file_close(afc, fh);
		}
		fclose(fd);
	} else
		return -1;
	return 0;
}

void minst_client(const char *operation, plist_t status, void *unused) {
	cb++;
	if (cb == 8) {
	}
	if (status && operation) {
		plist_t npercent = plist_dict_get_item(status, "PercentComplete");
		plist_t nstatus = plist_dict_get_item(status, "Status");
		plist_t nerror = plist_dict_get_item(status, "Error");
		int percent = 0;
		char *status_msg = NULL;
		if (npercent) {
			uint64_t val = 0;
			plist_get_uint_val(npercent, &val);
			percent = val;
		}
		if (nstatus) {
			plist_get_string_val(nstatus, &status_msg);
			if (!strcmp(status_msg, "Complete")) {
				sleep(1);
				installing = 0;
			}
		}
        
		if (nerror) {
			char *err_msg = NULL;
			plist_get_string_val(nerror, &err_msg);
			free(err_msg);
			installing = 0;
			installError = 1;
		}
	} else {
	}
}

void help(void){
    printf("%sHELP \n", KYEL);
    printf("%s    [*] IPA Path from pc or mac | Name of IPA \n", KYEL);
    printf("%s    [*] Example: \n", KYEL);
    printf("%s    [*] ./openiapp /Users/3x7R00Tripper/Desktop/app.ipa app.ipa \n", KYEL);
    printf("%s", KNRM);
}
void home(void) {
    printf("%s", KNRM);
    system("chmod +x resources/home");
    system("./resources/home");
    printf("    [*] Welcome to openiapp\n");
    printf("%s    [*] Thanks for using openiapp, opensource IPA App installer program for iOS devices by @3x7R00Tripper.\n", KMAG);
    printf("%s    [*] Maybe you use a tool wich use openiapp's source code or you run a compiled version of openiapp.\n", KCYN);
    printf("%s", KNRM);
}
int main(int argc, char *argv[]) {
    
    home();
    
    if(argc == 2 && strcmp(argv[1], "--help")==0)
    {
        help();
        return -1;
    }
    else if(argc == 2 && strcmp(argv[1], "-help")==0){
        help();
        return -1;
    }
    else if(argc == 2 && strcmp(argv[1], "-h")==0){
        help();
        return -1;
    }
    if(argc == 3)
    {
        char *dotipa = argv[1];
        
        char *dotipa2 = argv[2];
        
        char *var="Downloads/";
        char name[255];
        
        strcpy(name, var);
        strcat(name, dotipa2);
        
        idevice_error_t idevice_error = 0;
        idevice_error = idevice_new(&idevice, NULL);
        if (idevice_error != IDEVICE_E_SUCCESS) {
            return -1;
        }
        lockdownd_error_t lockdown_error = 0;
        lockdown_error = lockdownd_client_new_with_handshake(idevice, &lockdownd_client, "openiapp");
        if (lockdown_error != LOCKDOWN_E_SUCCESS) {
            return -1;
        }
        lockdownd_service_descriptor_t lsd = NULL;
        lockdown_error = lockdownd_start_service(lockdownd_client, "com.apple.afc", &lsd);
        if (lockdown_error != LOCKDOWN_E_SUCCESS) {
            return -1;
        }
        afc_error_t afc_do_it = 0;
        afc_do_it = afc_client_new(idevice, lsd, &afc_client);
        if (afc_do_it != AFC_E_SUCCESS) {
            lockdownd_client_free(lockdownd_client);
            idevice_free(idevice);
            return -1;
        }
        lockdownd_client_free(lockdownd_client);
        lockdownd_client = NULL;
        
        afc_do_it = afc_send_file(afc_client, dotipa, name);
        if (afc_do_it != AFC_E_SUCCESS) {
            printf("%s    [*] Error with sending file!\n", KRED);
            printf("%s", KNRM);
        }
        else {
            printf("%s    [*] Successfully sended file\n", KGRN);
            printf("%s", KNRM);
        }
        
        lockdown_error = lockdownd_client_new_with_handshake(idevice, &lockdownd_client,
                                                    "openiapp");
        if (lockdown_error != LOCKDOWN_E_SUCCESS) {
            return -1;
        }
        
        lockdown_error = lockdownd_start_service(lockdownd_client, "com.apple.mobile.installation_proxy", &lsd);
        if (lockdown_error != LOCKDOWN_E_SUCCESS) {
            lockdownd_client_free(lockdownd_client);
            return -1;
        }
        
        instproxy_error_t install_error = 0;
        instproxy_client_t install_proxy = NULL;
        install_error = instproxy_client_new(idevice, lsd, &install_proxy);
        if (install_error != INSTPROXY_E_SUCCESS) {
            return -1;
        }
        
        plist_t plist = instproxy_client_options_new();
        install_error = instproxy_install(install_proxy, name, plist, &minst_client, NULL );
        if (install_error != INSTPROXY_E_SUCCESS) {
            printf("%s    [*] Error with installing App\n", KRED);
            printf("%s", KNRM);
            return -1;
            instproxy_client_options_free(plist);
            instproxy_client_free(install_proxy);
        }
        
        if (installError) {
            return -1;
        }
 
            while (installing) {
                printf("%s    [*] Check if the app is installed successfully.\n", KGRN);
                printf("%s    [*] If the app is not installed, the installing proccess is running now and you must wait a short time\n", KGRN);
                printf("%s", KNRM);
            return -1;
            }
    }
    return 0;
}
