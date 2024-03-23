//
// Created by gtoscano on 4/1/23.
//

#ifndef CBO_EVALUATION_AMQP_CPP_H
#define CBO_EVALUATION_AMQP_CPP_H
#include <string>
#include <unordered_map>

#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include <sw/redis++/redis++.h>

class RabbitMQClient {
public:
    RabbitMQClient(std::string emo_data, std::string emo_uuid);
    RabbitMQClient();
    void init(std::string emo_data, std::string emo_uuid);
    ~RabbitMQClient();
    void get_opts();
    bool send_message(std::string routing_name, std::string msg);
    void send_signal(std::string exec_uuid);
    std::string wait_for_data();
    std::vector<std::string> wait_for_all_data();
    std::vector<std::string> safe_wait_for_all_data(); 
    int transfers_remaining();
    bool is_init();

private:
    AmqpClient::Channel::OpenOpts opts_;
    sw::redis::Redis redis_;
    std::string emo_uuid_;
    std::string emo_data_;
    std::unordered_map<std::string, std::string> sent_list_;
    bool is_initialized;
};

#endif //CBO_EVALUATION_AMQP_CPP_H
