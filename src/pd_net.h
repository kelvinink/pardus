#ifndef PD_NET_H
#define PD_NET_H

#define LISTENQ 1024

int pd_connect(const std::string& hostname, const std::string& port);
int pd_listen(const std::string& port);

#endif //PD_NET_H