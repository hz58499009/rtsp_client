#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "rtspCommon.h"

// Used to implement "RTSPOptionIsSupported()":
static int32_t isSeparator(char c) { return c == ' ' || c == ',' || c == ';' || c == ':'; }

int32_t RTSPOptionIsSupported(char const* commandName, char const* optionsResponseString) {
  do {
    if (commandName == NULL || optionsResponseString == NULL) break;

    unsigned const commandNameLen = strlen(commandName);
    if (commandNameLen == 0) break;

    // "optionsResponseString" is assumed to be a list of command names, separated by " " and/or ",", ";", or ":"
    // Scan through these, looking for "commandName".
    while (1) {
      // Skip over separators:
      while (*optionsResponseString != '\0' && isSeparator(*optionsResponseString)) ++optionsResponseString;
      if (*optionsResponseString == '\0') break;

      // At this point, "optionsResponseString" begins with a command name (with perhaps a separator afterwads).
      if (strncmp(commandName, optionsResponseString, commandNameLen) == 0) {
	// We have at least a partial match here.
	optionsResponseString += commandNameLen;
	if (*optionsResponseString == '\0' || isSeparator(*optionsResponseString)) return True;
      }

      // No match.  Skip over the rest of the command name:
      while (*optionsResponseString != '\0' && !isSeparator(*optionsResponseString)) ++optionsResponseString;
    }
  } while (0);

  return False;
}



int32_t ParseUdpPort(char *buf, uint32_t size, RtspSession *sess)
{
    /* Session ID */
    char *p = strstr(buf, SETUP_CPORT);
    if (!p) {
        printf("SETUP: %s not found\n", SETUP_CPORT);
        return False;
    }
    p = strchr((const char *)p, '=');
    if (NULL == p){
        printf("SETUP: = not found\n");
        return False;
    }
    char *sep = strchr((const char *)p, '-');
    if (NULL == sep){
        printf("SETUP: - not found\n");
        return False;
    }
    char tmp[8] = {0x00};
    strncpy(tmp, p+1, sep-p-1);
    sess->transport.udp.cport_from = atol(tmp);
    memset(tmp, 0x00, sizeof(tmp));


    p = strchr((const char *)sep, ';');
    if (NULL == p){
        printf("SETUP: = not found\n");
        return False;
    }
    strncpy(tmp, sep+1, p-sep-1);
    sess->transport.udp.cport_to = atol(tmp);
    memset(tmp, 0x00, sizeof(tmp));


    p = strstr(buf, SETUP_SPORT);
    if (!p) {
        printf("SETUP: %s not found\n", SETUP_SPORT);
        return False;
    }
    p = strchr((const char *)p, '=');
    if (NULL == p){
        printf("SETUP: = not found\n");
        return False;
    }
    sep = strchr((const char *)p, '-');
    if (NULL == sep){
        printf("SETUP: - not found\n");
        return False;
    }
    strncpy(tmp, p+1, sep-p-1);
    sess->transport.udp.sport_from = atol(tmp);
    memset(tmp, 0x00, sizeof(tmp));

    p = strchr((const char *)sep, ';');
    if (NULL == p){
        p = strstr((const char *)sep, "\r\n");
        if (NULL == p){
            printf("SETUP: %s not found\n", "\r\n");
            return False;
        }
    }
    strncpy(tmp, sep+1, p-sep-1);
    sess->transport.udp.sport_to = atol(tmp);
#ifdef RTSP_DEBUG
    printf("client port from %d to %d\n", \
            sess->transport.udp.cport_from, \
            sess->transport.udp.cport_to);
    printf("server port from %d to %d\n", \
            sess->transport.udp.sport_from, \
            sess->transport.udp.sport_to);
#endif
    return True;
}

int32_t ParseTimeout(char *buf, uint32_t size, RtspSession *sess)
{
    char *p = strstr(buf, TIME_OUT);
    if (!p) {
        printf("GET_PARAMETER: %s not found\n", SETUP_SESSION);
        return False;
    }
    p = strchr((const char *)p, '=');
    if (!p) {
        printf("GET_PARAMETER: '=' not found\n");
        return False;
    }
    char *sep = strchr((const char *)p, ';');
    if (NULL == sep){
        sep = strstr((const char *)p, "\r\n");
        if (NULL == sep){
            printf("GET_PARAMETER: %s not found\n", "\r\n");
            return False;
        }
    }

    char tmp[8] = {0x00};
    strncpy(tmp, p+1, sep-p-1);
    sess->timeout = atol(tmp);
#ifdef RTSP_DEBUG
    printf("timeout : %d\n", sess->timeout);
#endif
    return True;
}

int32_t ParseSessionID(char *buf, uint32_t size, RtspSession *sess)
{
    /* Session ID */
    char *p = strstr(buf, SETUP_SESSION);
    if (!p) {
        printf("SETUP: %s not found\n", SETUP_SESSION);
        return False;
    }
    p = strchr((const char *)p, ' ');
    if (!p) {
        printf("SETUP: ' ' not found\n");
        return False;
    }

    char *sep = strstr((const char *)p, "\r\n");
    if (NULL == sep){
        printf("SETUP: %s not found\n", "\r\n");
        return False;
    }

    char *nsep = strchr((const char *)p, ';');
    if (NULL == nsep){
        printf("SETUP: %s not found\n", ";");
        return False;
    }

    if (nsep < sep){
        memset(sess->sessid, '\0', sizeof(sess->sessid));
        memcpy((void *)sess->sessid, (const void *)p+1, nsep-p-1);
    }else if (nsep > sep){
        memset(sess->sessid, '\0', sizeof(sess->sessid));
        memcpy((void *)sess->sessid, (const void *)p+1, sep-p-1);
    }
#ifdef RTSP_DEBUG
    printf("sessid : %s\n", sess->sessid);
#endif
    return True;
}


int32_t ParseInterleaved(char *buf, uint32_t num, RtspSession *sess)
{
    char *p = strstr(buf, TCP_INTERLEAVED);
    if (!p) {
        printf("SETUP: %s not found\n", TCP_INTERLEAVED);
        return False;
    }
    p = strchr((const char *)p, '=');
    if (NULL == p){
        printf("SETUP: = not found\n");
        return False;
    }
    char *sep = strchr((const char *)p, '-');
    if (NULL == sep){
        printf("SETUP: - not found\n");
        return False;
    }
    char tmp[8] = {0x00};
    strncpy(tmp, p+1, sep-p-1);
    sess->transport.tcp.start = atol(tmp);
    memset(tmp, 0x00, sizeof(tmp));

    p = strchr((const char *)sep, ';');
    if (NULL == p){
        p = strstr((const char *)sep, "\r\n");
        if (NULL == p){
            printf("SETUP: %s not found\n", "\r\n");
            return False;
        }
    }

    strncpy(tmp, sep+1, p-sep-1);
    sess->transport.udp.cport_to = atol(tmp);
    memset(tmp, 0x00, sizeof(tmp));

#ifdef RTSP_DEBUG
    printf("tcp interleaved from %d to %d\n", \
            sess->transport.tcp.start, \
            sess->transport.tcp.end);
#endif
    return True;
}


void RtspIncreaseCseq(RtspSession *sess)
{
    sess->cseq++;
    return;
}

int32_t RtspResponseStatus(char *response)
{
    int32_t offset = sizeof(RTSP_RESPONSE) - 1;
    char buf[8], *sep = NULL;

    if (strncmp((const char*)response, (const char*)RTSP_RESPONSE, offset) != 0) {
        return -1;
    }

    sep = strchr((const char *)response+offset, ' ');
    if (!sep) {
        return -1;
    }

    memset(buf, '\0', sizeof(buf));
    strncpy((char *)buf, (const char *)(response+offset), sep-response-offset);

    return atoi(buf);
}

void GetSdpVideoAcontrol(char *buf, uint32_t size, RtspSession *sess)
{
    char *ptr = (char *)memmem((const void*)buf, size,
            (const void*)SDP_M_VIDEO, strlen(SDP_M_VIDEO)-1);
    if (NULL == ptr){
        fprintf(stderr, "Error: m=video not found!\n");
        return;
    }

    ptr = (char *)memmem((const void*)ptr, size,
            (const void*)SDP_A_CONTROL, strlen(SDP_A_CONTROL)-1);
    if (NULL == ptr){
        fprintf(stderr, "Error: a=control not found!\n");
        return;
    }

    char *endptr = (char *)memmem((const void*)ptr, size,
            (const void*)"\r\n", strlen("\r\n")-1);
    if (NULL == endptr){
        fprintf(stderr, "Error: %s not found!\n", "\r\n");
        return;
    }
    ptr += strlen(SDP_A_CONTROL);
    if ('*' == *ptr){
        /* a=control:* */
        printf("a=control:*\n");
        return;
    }else{
        /* a=control:rtsp://ip:port/track1  or a=control : TrackID=1*/
        memcpy((void *)sess->vmedia.control, (const void*)(ptr), (endptr-ptr));
        sess->vmedia.control[endptr-ptr] = '\0';
    }

    return;
}

void GetSdpVideoTransport(char *buf, uint32_t size, RtspSession *sess)
{
    char *ptr = (char *)memmem((const void*)buf, size,
            (const void*)SDP_M_VIDEO, strlen(SDP_M_VIDEO)-1);
    if (NULL == ptr){
        fprintf(stderr, "Error: m=video not found!\n");
        return;
    }

    ptr = (char *)memmem((const void*)ptr, size,
            (const void*)UDP_TRANSPORT, strlen(UDP_TRANSPORT)-1);
    if (NULL != ptr){
        sess->trans = RTP_AVP_UDP;
    }else{
        ptr = (char *)memmem((const void*)ptr, size,
                (const void*)TCP_TRANSPORT, strlen(TCP_TRANSPORT)-1);
        if (NULL != ptr)
            sess->trans = RTP_AVP_TCP;
    }

    return;
}

int32_t ParseSdpProto(char *buf, uint32_t size, RtspSession *sess)
{
    GetSdpVideoTransport(buf, size, sess);
    GetSdpVideoAcontrol(buf, size, sess);
#ifdef RTSP_DEBUG
    printf("video control: %s\n", sess->vmedia.control);
#endif
    return True;
}

uint32_t GetSDPLength(char *buf, uint32_t size)
{
    char *p = strstr(buf, "Content-Length: ");
    if (!p) {
        printf("Describe: Error, Transport header not found\n");
        return 0x00;
    }

    char *sep = strchr(p, ' ');
    if (NULL == sep)
        return 0x00;

    p = strstr(sep, "\r\n");
    if (!p) {
        printf("Describe : Content-Length: not found\n");
        return 0x00;
    }

    char tmp[8] = {0x00};
    strncpy(tmp, sep+1, p-sep-1);
    return  atol(tmp);
}
