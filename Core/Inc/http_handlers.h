#ifndef __HTTP_HANDLERS_H__
#define __HTTP_HANDLERS_H__

#include "lwip/apps/httpd.h"
#include "lwip/apps/fs.h"

void http_handlers_init(void);
void update_ssi_data(void);

// Объявление требуется для линковщика
void httpd_cgi_handler(struct fs_file *file, const char* uri, 
                       int iNumParams, char **pcParam, char **pcValue);

#endif /* __HTTP_HANDLERS_H__ */