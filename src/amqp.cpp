//
// Created by gtoscano on 4/1/23.
//
#include "amqp.h"
#include "misc_utilities.h"
#include <iostream>
#include <string>

#include <fmt/core.h>
#include <sw/redis++/redis++.h>

namespace {
    std::string EXCHANGE_NAME = "opt4cast_exchange";
    std::string AMQP_HOST = misc_utilities::get_env_var("AMQP_HOST", "127.0.0.1");
    std::string AMQP_USERNAME = misc_utilities::get_env_var("AMQP_USERNAME", "guest");
    std::string AMQP_PASSWORD= misc_utilities::get_env_var("AMQP_PASSWORD", "guest");
    std::string AMQP_PORT = misc_utilities::get_env_var("AMQP_PORT", "5672");
    std::string OPT4CAST_WAIT_MILLISECS_IN_CAST = misc_utilities::get_env_var("OPT4CAST_WAIT_MILLISECS_IN_CAST", "3600000");
    std::string REDIS_HOST = misc_utilities::get_env_var("REDIS_HOST", "127.0.0.1");
    std::string REDIS_PORT = misc_utilities::get_env_var("REDIS_PORT", "6379");
    std::string REDIS_DB_OPT = misc_utilities::get_env_var("REDIS_DB_OPT", "1");
    std::string REDIS_URL = fmt::format("tcp://{}:{}/{}", REDIS_HOST, REDIS_PORT, REDIS_DB_OPT);
}

RabbitMQClient::RabbitMQClient(std::string emo_data, std::string emo_uuid) : redis_(REDIS_URL) {
    init(emo_data, emo_uuid);
}

RabbitMQClient::RabbitMQClient(): redis_(REDIS_URL) {
    is_initialized = false;

}

void RabbitMQClient::init(std::string emo_data, std::string emo_uuid) {
    get_opts();
    emo_data_= emo_data;
    emo_uuid_ = emo_uuid;
    redis_.hset("emo_data", emo_uuid_, emo_data_);
    is_initialized = true;
}

bool RabbitMQClient::is_init() {
    return is_initialized;
}

RabbitMQClient::~RabbitMQClient() {
    redis_.hdel("emo_data", emo_uuid_);
    if(redis_.exists(emo_uuid_)) {
        redis_.del(emo_uuid_);
    }
}
void RabbitMQClient::get_opts() {
    opts_.host = AMQP_HOST.c_str();
    opts_.port = std::stoi(AMQP_PORT);
    opts_.vhost = "/";
    opts_.frame_max = 131072;
    opts_.auth = AmqpClient::Channel::OpenOpts::BasicAuth(AMQP_USERNAME, AMQP_PASSWORD);
}

bool RabbitMQClient::send_message(std::string routing_name, std::string msg) {
    try {
        auto channel = AmqpClient::Channel::Open(opts_);
        channel->DeclareExchange(EXCHANGE_NAME, AmqpClient::Channel::EXCHANGE_TYPE_DIRECT, false, true, true);
        auto message = AmqpClient::BasicMessage::Create(msg);
        channel->BasicPublish(EXCHANGE_NAME, routing_name, message, false, false);
        //channel->reset()
    }
    catch (const std::exception &error) {
        std::clog << "error\n" << error.what() << "\n";
        std::cerr << "error\n" << error.what() << "\n";
    }
    return true;
}

void RabbitMQClient::send_signal(std::string exec_uuid) {
    redis_.hset("emo_data", exec_uuid, emo_data_);
    auto scenario_id = *redis_.lpop("scenario_ids");
    //std::cout<<"Current Scenario ID: "<<scenario_id<<std::endl;
    redis_.hset("solution_to_execute_dict", exec_uuid, fmt::format("{}_{}", emo_uuid_, scenario_id));
    try {
        auto msg = exec_uuid;
        auto routing_name = "opt4cast_execution";
        send_message(routing_name, msg);
        sent_list_[exec_uuid] = scenario_id;
    }
    catch (const std::exception &error) {
        std::cerr << "Error in evaluate parallel " << error.what() << std::endl;
    }
}

std::string RabbitMQClient::wait_for_data() {
    std::string exec_results_str;
    auto channel = AmqpClient::Channel::Open(opts_);
    channel->DeclareExchange(EXCHANGE_NAME, AmqpClient::Channel::EXCHANGE_TYPE_DIRECT, false, true, true);
    auto queue_name = channel->DeclareQueue("", false, true, true, true);
    channel->BindQueue(queue_name, EXCHANGE_NAME, emo_uuid_);
    auto consumer_tag = channel->BasicConsume(queue_name, "", true, true, true, 1);


    fmt::print("[*] Waiting for execution service: {} \n", emo_uuid_);

    auto envelop = channel->BasicConsumeMessage(consumer_tag);
    auto message_payload = envelop->Message()->Body();
    auto routing_key = envelop->RoutingKey();
    std::string received_exec_uuid = message_payload;

    if (routing_key == emo_uuid_) {
        exec_results_str = *redis_.hget("executed_results", received_exec_uuid);
        auto scenario_id = sent_list_[received_exec_uuid];
        sent_list_.erase(received_exec_uuid);
        //redis_.lpush("scenario_ids", scenario_id);
        redis_.hdel("executed_results", received_exec_uuid);
        redis_.hdel("emo_data", received_exec_uuid);
    }


    return exec_results_str;
}
std::vector<std::string> RabbitMQClient::wait_for_all_data() {
    std::vector<std::string> exec_results_str_all;
    auto channel = AmqpClient::Channel::Open(opts_);
    channel->DeclareExchange(EXCHANGE_NAME, AmqpClient::Channel::EXCHANGE_TYPE_DIRECT, false, true, true);
    auto queue_name = channel->DeclareQueue("", false, true, true, true);
    channel->BindQueue(queue_name, EXCHANGE_NAME, emo_uuid_);
    auto consumer_tag = channel->BasicConsume(queue_name, "", true, true, true, 1);

    while(sent_list_.size() > 0) {

        fmt::print("Remaining scenarios: {}\n", sent_list_.size());
        fmt::print("[*] Waiting for execution service: {} \n", emo_uuid_);

        auto envelop = channel->BasicConsumeMessage(consumer_tag);
        auto message_payload = envelop->Message()->Body();
        auto routing_key = envelop->RoutingKey();
        std::string received_exec_uuid = message_payload;

        if (routing_key == emo_uuid_) {
            std::string exec_results_str = *redis_.hget("executed_results", received_exec_uuid);
            std::vector<std::string> to_return = {received_exec_uuid, exec_results_str};
            exec_results_str_all.push_back(fmt::format("{}_{}", received_exec_uuid, exec_results_str));
            auto scenario_id = sent_list_[received_exec_uuid];
            sent_list_.erase(received_exec_uuid);
            //redis_.lpush("scenario_ids", scenario_id);
            redis_.hdel("executed_results", received_exec_uuid);
            redis_.hdel("emo_data", received_exec_uuid);
        }
    }


    return exec_results_str_all;
}


int RabbitMQClient::transfers_remaining() {
    return sent_list_.size();
}
