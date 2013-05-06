#ifndef CUSTOMCODECS20130506H
#define CUSTOMCODECS20130506H

#include <vector>
#include <map>
#include <cassert>

#include <boost/algorithm/string.hpp>

#include "procman-analogs.pb.h"

#include "goby/acomms/dccl.h"
#include "goby/common/logger.h"

#include <lcmtypes/bot_procman/orders_t.hpp>
#include <lcmtypes/bot_procman/info_t.hpp>

enum Node { BASE = 0, ROBOT = 1};

class CustomChannelCodec
{
  public:
    CustomChannelCodec() { }
    virtual ~CustomChannelCodec() { }
    
    // lcm_data is the LCM encoded message, transmit_data is a pointer to store whatever data this custom codec
    // wishes to transmit. Returns true if data is provided in transmit_data, returns false if this message is to be discarded
    virtual bool encode(const std::vector<unsigned char>& lcm_data, std::vector<unsigned char>* transmit_data) = 0;
    // undo encode. Returns false if this message cannot be decoded, true otherwise.
    virtual bool decode(std::vector<unsigned char>* lcm_data, const std::vector<unsigned char>& transmit_data) = 0;
};

template<typename LCMType, typename DiffType, typename Codec, typename OtherCodec>
    class PMDWrapperCodec : public CustomChannelCodec
    {
      public:
      PMDWrapperCodec(Node node)
          : node_(node),
            dccl_(goby::acomms::DCCLCodec::get())
        {
//            dccl_->validate<DiffType>();
        }
        
        bool encode(const std::vector<unsigned char>& lcm_data, std::vector<unsigned char>* transmit_data)
        {
            using goby::glog;
            using namespace goby::common::logger;

            LCMType lcm_object;
            if(lcm_object.decode(&lcm_data[0], 0, lcm_data.size()) == -1)
                return false;
            
            drc::ProcManWrapper wrapper;

            drc::ProcManWrapper::Host host;
            if(!drc::ProcManWrapper::Host_Parse(boost::to_upper_copy(lcm_object.host), &host))
            {
                glog.is(WARN) && glog << "Warning, could not parse Host: " << lcm_object.host <<". Make sure it is defined in procman-analogs.proto" << std::endl;
                return false;
            }
            else if(host == drc::ProcManWrapper::BASE && node() == BASE)
            {
                glog.is(VERBOSE) && glog << "Not sending BASE procman messages to other nodes" << std::endl;
                return false;
            }            

            wrapper.set_host(host);

            // if we have nothing to compare to, or we need to signal the other side to
            // send a full message
            if(!Codec::state_.count(lcm_object.host) || !OtherCodec::state_.count(lcm_object.host))
            {
                if(!OtherCodec::state_.count(lcm_object.host))
                    wrapper.set_request_full(true);
                    
                Codec::state_[lcm_object.host] = lcm_object;
                wrapper.set_type(drc::ProcManWrapper::FULL);
                
                wrapper.mutable_data()->resize(lcm_object.getEncodedSize());
                lcm_object.encode(&(*wrapper.mutable_data())[0], 0, wrapper.data().size());

            }
            else
            {
                DiffType diff;
                wrapper.set_type(drc::ProcManWrapper::DIFF);
                if(!make_diff(lcm_object, Codec::state_[lcm_object.host], &diff))
                    return false;

                diff.SerializeToString(wrapper.mutable_data());
//                dccl_->encode(wrapper.mutable_data(), diff);
            }

            glog.is(VERBOSE) && glog << "ProcMan Message type is: "
                                     << drc::ProcManWrapper::MessageType_Name(wrapper.type())
                                     << " with host: "
                                     << drc::ProcManWrapper::Host_Name(wrapper.host()) << std::endl;
            
            transmit_data->resize(wrapper.ByteSize());
            wrapper.SerializeToArray(&(*transmit_data)[0], transmit_data->size());
            
            return true;
        }
        
        bool decode(std::vector<unsigned char>* lcm_data, const std::vector<unsigned char>& transmit_data)
        {
            using goby::glog;
            using namespace goby::common::logger;

            drc::ProcManWrapper wrapper;
            wrapper.ParseFromArray(&transmit_data[0], transmit_data.size());
    
            LCMType lcm_object;

            if(wrapper.type() == drc::ProcManWrapper::FULL)
            {    
                lcm_object.decode(&wrapper.data()[0], 0, wrapper.data().size());        

                // if we already have a full message,
                // assume this is a signal to reset the full messages
                if(wrapper.request_full())
                {
                    glog.is(VERBOSE) && glog << "Other side requested FULL message, resetting state" << std::endl;
                    OtherCodec::state_.erase(lcm_object.host);
                }
                
                Codec::state_[lcm_object.host] = lcm_object;

            }
            else if(wrapper.type() == drc::ProcManWrapper::DIFF)
            {
                std::string host = drc::ProcManWrapper::Host_Name(wrapper.host());
                boost::to_lower(host);
                if(!Codec::state_.count(host))
                    return false;
                
                DiffType diff;
                diff.ParseFromString(wrapper.data());
//                dccl_->decode(wrapper.data(), &diff);

                if(!reverse_diff(&lcm_object, Codec::state_[host], diff))
                {
                    // assume our reference is no good, so delete it and force the other side to send a new one
                    Codec::state_.erase(host);
                    return false;
                }
                
            }
    
            lcm_data->resize(lcm_object.getEncodedSize());
            lcm_object.encode(&(*lcm_data)[0], 0, lcm_data->size());
    
            return true;
    
        }

        Node node() const { return node_; }
      protected:
        virtual bool make_diff(const LCMType& lcm_object, const LCMType& reference,
                               DiffType* diff) = 0;
        virtual bool reverse_diff(LCMType* lcm_object, const LCMType& reference,
                                  const DiffType& diff) = 0;

      private:
        Node node_;        
        goby::acomms::DCCLCodec* dccl_;
    };


class PMDInfoCodec;

class PMDOrdersCodec : public PMDWrapperCodec<bot_procman::orders_t, drc::PMDOrdersDiff, PMDOrdersCodec, PMDInfoCodec>
{
  public:
  PMDOrdersCodec(Node node)
      : PMDWrapperCodec(node)
    {
        static bool only = true;
        assert(only);
        only = false;
    }

    bool make_diff(const bot_procman::orders_t& orders, const bot_procman::orders_t& reference,
                   drc::PMDOrdersDiff* diff);
    bool reverse_diff(bot_procman::orders_t* orders, const bot_procman::orders_t& reference,
                      const drc::PMDOrdersDiff& diff);
    
    // maps host to latest orders_t
    static std::map<std::string, bot_procman::orders_t> state_;    
};


class PMDInfoCodec : public PMDWrapperCodec<bot_procman::info_t, drc::PMDInfoDiff, PMDInfoCodec, PMDOrdersCodec>
{
  public:
  PMDInfoCodec(Node node)
      : PMDWrapperCodec(node)
    {
        static bool only = true;
        assert(only);
        only = false;
    }

    bool make_diff(const bot_procman::info_t& info, const bot_procman::info_t& reference,
                   drc::PMDInfoDiff* diff);
    bool reverse_diff(bot_procman::info_t* info, const bot_procman::info_t& reference,
                      const drc::PMDInfoDiff& diff);

    // maps host to latest info_t
    static std::map<std::string, bot_procman::info_t> state_;
};


#endif
