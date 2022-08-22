
#include <amm_std.h>
#include <tinyxml2.h>
#include <string>
#include <vector>

#include "amm/BaseLogger.h"

class event_sequence {

    struct event {
        int asmt;
        std::string name;
    };

    int mode = 0;
    int current_frame = 0;
    std::vector <std::string> event_list;
    std::vector <event> received_event_list;

public:
    event_sequence(int mode, std::vector <std::string> event_list) {
        this->mode = mode;
        this->event_list = event_list;
        received_event_list = std::vector<event>();
    }

    /// asmt 0 = Success
    /// asmt 1 = Commission
    /// asmt 2 = Omission
    void assess_event(const std::string evnt, int &asmt) {

        /// Event struct only gets an Omission assessment if the mode is non-sequential.
        /// Sequential mode does another check at the end.

        /// Non-sequential
        if (mode == 1) {
            asmt = 1;
            for (auto exp_evnt : event_list) {
                /// Scan for a match and set Success if match.
                if (evnt == exp_evnt) {
                    asmt = 0;
                    break;
                }
            }
        }

            /// Check for sequence runover if squential list.
        else if (mode == 2 && current_frame >= event_list.size()) {
            /// Set Comission Error because number of incoming events has surpassed the expected sequence.
            asmt = 1;
        }

            /// Sequential
        else if (mode == 2 && evnt == event_list[current_frame]) {
            /// Set Success and move to next frame in the expected sequence.
            asmt = 0;
            current_frame++;
        } else {
            /// Set Commission Error and mdoe to next frame in the expected until a match is found.
            asmt = 1;
            for (int i = current_frame; i < event_list.size(); i++) {
                if (evnt == event_list[i]) {
                    /// If match, move to the next frame after the match.
                    current_frame = i + 1;
                    break;
                }
            }
        }

        event evnt_stc{asmt, evnt};
        received_event_list.push_back(evnt_stc);
    }

    void scan_for_omissions(std::vector <std::string> &omitted_events) {
        omitted_events = std::vector<std::string>();

        for (auto exp_evnt : event_list) {
            int asmt = 2;
            for (auto rcv_evnt : received_event_list) {
                if (exp_evnt == rcv_evnt.name) {
                    asmt = rcv_evnt.asmt;
                    break;
                }
            }
            if (asmt == 2) {
                omitted_events.push_back(exp_evnt);
            }
        }
    }
};

bool isRunning = true;
AMM::UUID module_id;

/// asmt_mode 0 : Uninitialized.
/// asmt_mode 1 : Not sequential.
/// asmt_mode 2 : Is sequential.
int asmt_mode = 0;
const std::string sysPrefix = "[SYS]";
event_sequence *evnt_seq;
AMM::DDSManager<void> *mgr = new AMM::DDSManager<void>("config/simple_assessment_amm.xml");

void read_assessment(tinyxml2::XMLElement *node) {

    auto evnt_list = std::vector<std::string>();

    while (node) {
        LOG_INFO << "\tAdding " << node->Attribute("event") << " to event list.";
        evnt_list.push_back(node->Attribute("event"));
        node = node->NextSiblingElement();
    }

    evnt_seq = new event_sequence(asmt_mode, evnt_list);
}


void parse_xml(std::string &xmlConfig) {
    LOG_INFO << "Parsing XML from bus";
    tinyxml2::XMLDocument doc;
    doc.Parse(xmlConfig.c_str());
    tinyxml2::XMLNode *pRoot = doc.RootElement();
    if (!strcmp(pRoot->ToElement()->Attribute("name"), "simple_assessment")) {
        LOG_INFO << "Simple assessment capability";
    } else {
        LOG_WARNING << "Not a simple assessment, abort.";
        return;
    }

    tinyxml2::XMLElement *node = pRoot->FirstChildElement("configuration_data")->FirstChildElement("data");
    if (node == nullptr) {
        LOG_ERROR << "Unable to get Assessment configuration.";
        return;
    }

    if (!strcmp(node->Attribute("type"), "simple_assessment")) {
        LOG_INFO << "This is a simple assessment.";
    } else {
        LOG_WARNING << "Not a simple assessment.";
        return;
    }

    tinyxml2::XMLElement *ele = node->FirstChildElement("AssessmentList");

    if (!strcmp(ele->Attribute("type"), "sequential")) {
        LOG_INFO << "Assessment is sequential";
        asmt_mode = 1;
    } else {
        asmt_mode = 0;
    }

    read_assessment(ele->FirstChildElement("Assessment"));
}

void parse_xml_from_file(const std::string xmlFileName) {
    tinyxml2::XMLDocument doc;
    doc.LoadFile(xmlFileName.c_str());
    tinyxml2::XMLNode *node = doc.FirstChild();
    if (!strcmp(node->ToElement()->Attribute("type"), "sequential")) {
        asmt_mode = 1;
    } else {
        asmt_mode = 0;
    }

    tinyxml2::XMLElement *ele = node->FirstChildElement();
    read_assessment(ele);
}

int publish_asmt(int asmt_val, AMM::UUID evnt_id, std::string evnt_name) {

    int err;
    std::string errmsg;

    if (asmt_val == 2) {
        LOG_INFO << "Generating Omitted Event [" << evnt_name << "] (" << evnt_id.id() << ") ...";

        AMM::OmittedEvent oevnt;
        oevnt.id(evnt_id);
        oevnt.type(evnt_name);
        err = mgr->WriteOmittedEvent(errmsg, oevnt);
        if (err != 0) {
            LOG_ERROR << "ERR: Failed to write Omitted Event: " << errmsg;
        } else {
            LOG_INFO << "Omitted Event (" << evnt_id.id() << ") sent";
        }
    }

    if (asmt_val == 2 && err != 0) {
        LOG_ERROR << "ERR: Not generating Assessment because Omitted Event failed.";
        return err;
    }

    AMM::UUID asmt_id;
    asmt_id.id(mgr->GenerateUuidString());

    AMM::Assessment asmt;
    asmt.id(asmt_id);
    asmt.event_id(evnt_id);

    auto asmt_val_enum = AMM::AssessmentValue::SUCCESS;
    if (asmt_val == 1) asmt_val_enum = AMM::AssessmentValue::COMMISSION_ERROR;
    if (asmt_val == 2) asmt_val_enum = AMM::AssessmentValue::OMISSION_ERROR;
    asmt.value(asmt_val_enum);

    LOG_INFO << "Generating Assessment [" << asmt_val_enum << "] for Event (" << evnt_id.id() << ") ...";

    err = mgr->WriteAssessment(asmt);
    if (err != 0) {
        LOG_ERROR << "ERR: Failed to write Assessment: " << errmsg;
        return err;
    } else {
        LOG_INFO << "Assessment (" << asmt_id.id() << ") sent";
    }

    return 0;
}

/// TODO: Figure out what triggers this when the sim is over.
void on_simulation_end_signal(/* one or more params maybe */) {
    LOG_INFO << "End of simulation signaled, finishing up.";
    std::vector <std::string> omsn_list;
    evnt_seq->scan_for_omissions(omsn_list);
    for (auto omsn : omsn_list) {
        AMM::UUID evnt_id;
        evnt_id.id(mgr->GenerateUuidString());
        publish_asmt(2, evnt_id, omsn);
    }

    isRunning = false;
}

void on_event_record(AMM::EventRecord &evnt_rcd, SampleInfo_t *info) {
    LOG_DEBUG << "Event record received.";

    if (asmt_mode == 0) {
        LOG_ERROR << "ERR: Event Sequencer uninitialized. Cannot assess.";
        return;
    }

    /// asmt_val 0 = Success
    /// asmt_val 1 = Commission
    /// asmt_val 2 = Omission
    int asmt_val = -1;
    evnt_seq->assess_event(evnt_rcd.type(), asmt_val);

    if (asmt_val == -1) {
        LOG_ERROR << "Failure to assess Event Record [" << evnt_rcd.type() << "] (" << evnt_rcd.id().id() << ")";
        return;
    }

    publish_asmt(asmt_val, evnt_rcd.id(), evnt_rcd.type());
}

void on_command(AMM::Command &command, SampleInfo_t *info) {
    if (!command.message().compare(0, sysPrefix.size(), sysPrefix)) {
        std::string value = command.message().substr(sysPrefix.size());
        LOG_TRACE << "Received command: " << command.message();
        if (value.find("START_SIM") != std::string::npos) {

        } else if (value.find("STOP_SIM") != std::string::npos) {

        } else if (value.find("PAUSE_SIM") != std::string::npos) {

        } else if (value.find("RESET_SIM") != std::string::npos) {

        } else if (value.find("END_SIMULATION") != std::string::npos) {
            on_simulation_end_signal();
        }
    }
}

void on_module_configuration(AMM::ModuleConfiguration &mc, SampleInfo_t *info) {
    if (mc.name() == "simple_assessment") {
        LOG_TRACE << "Module config received, MC name is " << mc.name();
        parse_xml(mc.capabilities_configuration());
    }
}


void publish_operational_description() {
    LOG_INFO << "Publishing operational description.";
    AMM::OperationalDescription od;
    od.name("AJAMS Assessment Module");
    od.model("AJAMS Assessment Module");
    od.manufacturer("Vcom3D");
    od.serial_number("1.0.0");
    od.module_id(module_id);
    od.module_version("1.0.0");
    std::string capabilities = AMM::Utility::read_file_to_string("config/capability_config.xml");
    od.capabilities_schema(capabilities);
    od.description();
    mgr->WriteOperationalDescription(od);
}

int main() {
    static plog::ColorConsoleAppender <plog::TxtFormatter> consoleAppender;
    plog::init(plog::verbose, &consoleAppender);

    LOG_INFO << "AJAMS Assessment Module";

    mgr = new AMM::DDSManager<void>("config/simple_assessment_amm.xml");
    module_id.id(mgr->GenerateUuidString());

    mgr->InitializeEventRecord();
    mgr->InitializeModuleConfiguration();
    mgr->InitializeOperationalDescription();
    mgr->InitializeAssessment();
    mgr->InitializeOmittedEvent();
    mgr->InitializeCommand();

    mgr->CreateEventRecordSubscriber(&on_event_record);
    mgr->CreateModuleConfigurationSubscriber(&on_module_configuration);
    mgr->CreateCommandSubscriber(&on_command);

    mgr->CreateOperationalDescriptionPublisher();
    mgr->CreateAssessmentPublisher();
    mgr->CreateOmittedEventPublisher();

    publish_operational_description();

    LOG_INFO << "Waiting for assessment initialization...";

    auto loop = [&]() {
        for (;;) {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            if (!isRunning) break;
        }
    };

    std::thread t(loop);
    LOG_INFO << "AJAMS Assessment Module running... Press return to exit.";
    std::cin.get();
    isRunning = false;
    t.join();

    mgr->Shutdown();
    delete mgr;
    if (evnt_seq != NULL) delete evnt_seq;

    return 0;
}
