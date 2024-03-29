#ifndef _CLASSFY_HTTP_H_
#define _CLASSFY_HTTP_H_

#include "packet_processor.h"
#include <regex>
#include <string>

class ClassfyHTTP : public PacketProcessor {
public:
    ClassfyHTTP() {}
    void Init(const PacketProcessorConfig& pp_config) override;
    void Run() override;
    void FlushState() override;
    void RecoverState() override;
private:
    char *head;
    char *tail;
};

#endif // _CLASSFY_HTTP_H_
