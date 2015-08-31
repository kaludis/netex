#ifndef CFGREADER_H_
#define CFGREADER_H_

void read_config(const char* file_name);

const char* cmd_by_port(unsigned short port);

void clear_config();

#endif  /* CFGREADER_H_ */
