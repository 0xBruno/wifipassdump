#include <stdio.h>
#include <windows.h>
#include <wlanapi.h>
#include <string.h>
#pragma comment(lib, "Wlanapi.lib")
#pragma comment(lib, "Ole32.lib")

void getKeyMaterial(char *, char *);

int main(){
    printf("WIFIPASSDUMP\n");

    // open connection to server
    DWORD dwClientVersion = 2;
    DWORD pdwNegotiatedVersion = 0;
    HANDLE phClientHandle = NULL;
    
    DWORD openHandleStatus = WlanOpenHandle(dwClientVersion, NULL, &pdwNegotiatedVersion, &phClientHandle);
    
    // if not ERROR_SUCCESS
    if(openHandleStatus != ERROR_SUCCESS){ 
        return openHandleStatus; 
    }

    // enumerate wlan interfaces
    PWLAN_INTERFACE_INFO_LIST interfaces = NULL; 
    int err = WlanEnumInterfaces(phClientHandle, NULL, &interfaces);

    // if not ERROR_SUCCESS
    if(err != ERROR_SUCCESS){ 
        wprintf(L"WlanEnumInterfaces failed: %d\n", err); 
    }
    
    printf("[*] Enumerating wireless interface descriptions...\n");
    WCHAR GuidString[39] = {0};
    PWLAN_PROFILE_INFO_LIST pProfileList = NULL;
    PWLAN_PROFILE_INFO pProfile = NULL;

    for(int i = 0; i < (int) interfaces->dwNumberOfItems; i++){ 
        WLAN_INTERFACE_INFO wlanInterface = interfaces->InterfaceInfo[i];    
        wprintf(L"[*] INTERFACE %d: %s\n", i, wlanInterface.strInterfaceDescription);
        
        err = StringFromGUID2(&wlanInterface.InterfaceGuid, (LPOLESTR) &GuidString, sizeof(GuidString)/sizeof(*GuidString)); 
    
        if (err == ERROR_SUCCESS)
            wprintf(L"StringFromGUID2 failed: %d\n", err);
        else {
            wprintf(L"[*] GUID[%d]: %ws\n",i, GuidString);
        } 

        err = WlanGetProfileList(phClientHandle, &wlanInterface.InterfaceGuid, NULL, &pProfileList);

        if (err != ERROR_SUCCESS) { 
            wprintf(L"WlanGetProfileList failed: %d\n", err);
        } 

        int j; 
        LPWSTR profileXMLData;
        int pdwFlag = WLAN_PROFILE_GET_PLAINTEXT_KEY;
        char ptKey[50] = {0};

        for (int j = 0; j < pProfileList->dwNumberOfItems; j++) {
            pProfile = (WLAN_PROFILE_INFO *) & pProfileList->ProfileInfo[j];          
                       
            err = WlanGetProfile(phClientHandle, &wlanInterface.InterfaceGuid, pProfile->strProfileName, NULL, &profileXMLData, &pdwFlag, NULL);
            
            if (err != ERROR_SUCCESS) { 
                wprintf(L"WlanGetProfile failed: %d\n", err);
                continue;
            }
            
            char *xmlBuffer = (char *)malloc(wcslen(profileXMLData));
            wcstombs(xmlBuffer, profileXMLData, 1500);
            getKeyMaterial(xmlBuffer, ptKey);
            wprintf(L"\n%s:", pProfile->strProfileName);
            printf("%s", ptKey);
            printf("\n");
            free(xmlBuffer);
        }
  
        
    }
    

    
    WlanCloseHandle(phClientHandle, NULL);
    return 0; 

}


void getKeyMaterial(char* xmlBuffer, char *ptKey){
    char *ot;
    char *ct; 
    int si; 
    int ei; 
    char openTag[] = "<keyMaterial>";
    char closeTag[] = "</keyMaterial>";
    ot = strstr(xmlBuffer, openTag);
    ct = strstr(xmlBuffer, closeTag);
    si = strlen(xmlBuffer) - strlen(ot); 
    ei = strlen(xmlBuffer) - strlen(ct);

    // extract key between start index and end index of the open and closing tags
    int j = 0; 
    for(int i = si; i <= ei - strlen(closeTag); i++){
        ptKey[j] = xmlBuffer[i+strlen(openTag)];
        j++;
    }

    // NULL out values in plaintext key buffer that aren't the key
    int keyLen = j;
    for(int k = 0; k < strlen(ptKey); k++){ 
        if(k >= keyLen) { 
            ptKey[k] = '\0';
        }
    }
}