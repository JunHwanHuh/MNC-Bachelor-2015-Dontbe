#include "DFMessageWriter.h"

#include "DataFlash.h"

extern const AP_HAL::HAL& hal;

/* LogStartup - these are simple state machines which allow us to
 * trickle out messages to the log files
 */

void DFMessageWriter::reset()
{
    _finished = false;
}

void DFMessageWriter_DFLogStart::reset()
{
    DFMessageWriter::reset();

    _fmt_done = false;
    _writesysinfo.reset();
    _writeentiremission.reset();

    stage = ls_blockwriter_stage_init;
    next_format_to_send = 0;
    ap = AP_Param::first(&token, &type);
}

void DFMessageWriter_DFLogStart::process()
{
    switch(stage) {
    case ls_blockwriter_stage_init:
        stage = ls_blockwriter_stage_formats;
        // fall through

    case ls_blockwriter_stage_formats:
        // write log formats so the log is self-describing
        while (next_format_to_send < _DataFlash._num_types) {
            if (!_DataFlash.Log_Write_Format(&_DataFlash._structures[next_format_to_send])) {
                return; // call me again!
            }
            // provide hook to avoid corrupting the APM1/APM2
            // dataflash by writing too fast:
            _DataFlash.WroteStartupFormat();
            next_format_to_send++;
        }
        _fmt_done = true;
        stage = ls_blockwriter_stage_parms;
        // fall through

    case ls_blockwriter_stage_parms:
        while (ap) {
            if (!_DataFlash.Log_Write_Parameter(ap, token, type)) {
                return;
            }
            ap = AP_Param::next_scalar(&token, &type);
            _DataFlash.WroteStartupParam();
        }

        stage = ls_blockwriter_stage_sysinfo;
        // fall through

    case ls_blockwriter_stage_sysinfo:
        _writesysinfo.process();
        if (!_writesysinfo.finished()) {
            return;
        }
        stage = ls_blockwriter_stage_write_entire_mission;
        // fall through

    case ls_blockwriter_stage_write_entire_mission:
        _writeentiremission.process();
        if (!_writeentiremission.finished()) {
            return;
        }
        stage = ls_blockwriter_stage_vehicle_messages;
        // fall through

    case ls_blockwriter_stage_vehicle_messages:
        // we guarantee 200 bytes of space for the vehicle startup
        // messages.  This allows them to be simple functions rather
        // than e.g. DFMessageWriter-based state machines
        if (_DataFlash._vehicle_messages) {
            if (_DataFlash.bufferspace_available() < 200) {
                return;
            }
            _DataFlash._vehicle_messages();
        }
        stage = ls_blockwriter_stage_done;
        // fall through

    case ls_blockwriter_stage_done:
        break;
    }

    _finished = true;
}

void DFMessageWriter_WriteSysInfo::reset()
{
    DFMessageWriter::reset();
    stage = ws_blockwriter_stage_init;
}

void DFMessageWriter_DFLogStart::set_mission(const AP_Mission *mission)
{
    _writeentiremission.set_mission(mission);
}


void DFMessageWriter_WriteSysInfo::process() {
    switch(stage) {

    case ws_blockwriter_stage_init:
        stage = ws_blockwriter_stage_firmware_string;
        // fall through

    case ws_blockwriter_stage_firmware_string:
        if (! _DataFlash.Log_Write_Message(_firmware_string)) {
            return; // call me again
        }
        stage = ws_blockwriter_stage_git_versions;
        // fall through

    case ws_blockwriter_stage_git_versions:
#if defined(PX4_GIT_VERSION) && defined(NUTTX_GIT_VERSION)
        if (! _DataFlash.Log_Write_Message("PX4: " PX4_GIT_VERSION " NuttX: " NUTTX_GIT_VERSION)) {
            return; // call me again
        }
#endif
        stage = ws_blockwriter_stage_system_id;
        // fall through

    case ws_blockwriter_stage_system_id:
        char sysid[40];
        if (hal.util->get_system_id(sysid)) {
            if (! _DataFlash.Log_Write_Message(sysid)) {
                return; // call me again
            }
        }
        // fall through
    }

    _finished = true;  // all done!
}

// void DataFlash_Class::Log_Write_SysInfo(const char *firmware_string)
// {
//     DFMessageWriter_WriteSysInfo writer(firmware_string);
//     writer->process();
// }

void DFMessageWriter_WriteEntireMission::process() {
    switch(stage) {

    case em_blockwriter_stage_init:
        if (_mission == NULL) {
            stage = em_blockwriter_stage_done;
            break;
        } else {
            stage = em_blockwriter_stage_write_new_mission_message;
        }
        // fall through

    case em_blockwriter_stage_write_new_mission_message:
        if (! _DataFlash.Log_Write_Message("New mission")) {
            return; // call me again
        }
        stage = em_blockwriter_stage_write_mission_items;
        // fall through

    case em_blockwriter_stage_write_mission_items:
        AP_Mission::Mission_Command cmd;
        while (_mission_number_to_send < _mission->num_commands()) {
            // upon failure to write the mission we will re-read from
            // storage; this could be improved.
            if (_mission->read_cmd_from_storage(_mission_number_to_send,cmd)) {
                if (!_DataFlash.Log_Write_Mission_Cmd(*_mission, cmd)) {
                    return; // call me again
                }
            }
            _mission_number_to_send++;
        }
        stage = em_blockwriter_stage_done;
        // fall through

    case em_blockwriter_stage_done:
        break;
    }

    _finished = true;
}

void DFMessageWriter_WriteEntireMission::reset()
{
    DFMessageWriter::reset();
    stage = em_blockwriter_stage_init;
    _mission_number_to_send = 0;
}


void DFMessageWriter_WriteEntireMission::set_mission(const AP_Mission *mission)
{
    _mission = mission;
}
