#include "CommandHandler.h"
#include "helper.h"

namespace BrightnESS {
namespace FileWriter {

// In the future, want to handle many, but not right now.
static int g_N_HANDLED = 0;

CommandHandler::CommandHandler(Master * master) : master(master) {
	if (false) {
		using namespace rapidjson;
		auto buf1 = gulp(master->config.dir_assets + "/test/schema-command.json");
		auto doc = make_unique<rapidjson::Document>();
		ParseResult err = doc->Parse(buf1.data(), buf1.size());
		if (err.Code() != ParseErrorCode::kParseErrorNone) {
			LOG(7, "ERROR can not parse schema_command");
			throw std::runtime_error("ERROR can not parse schema_command");
		}
		schema_command.reset(new SchemaDocument(*doc));
	}
}

void CommandHandler::handle_new(rapidjson::Document & d) {
	if (g_N_HANDLED > 0) return;

	using namespace rapidjson;
	if (schema_command) {
		SchemaValidator vali(*schema_command);
		if (!d.Accept(vali)) {
			StringBuffer sb1, sb2;
			vali.GetInvalidSchemaPointer().StringifyUriFragment(sb1);
			vali.GetInvalidDocumentPointer().StringifyUriFragment(sb2);
			LOG(6, "ERROR command message schema validation:  Invalid schema: {}  keyword: {}",
				sb1.GetString(),
				vali.GetInvalidSchemaKeyword()
			);
			throw std::runtime_error("ERROR command message schema validation");
		}
	}

	LOG(1, "cmd: {}", d["cmd"].GetString());
	auto fwt = std::unique_ptr<FileWriterTask>(new FileWriterTask);
	std::string fname = "a-dummy-name.h5";
	if (d.HasMember("filename")) {
		if (d["filename"].IsString()) {
			fname = d["filename"].GetString();
		}
	}
	fwt->set_hdf_filename(fname);

	for (auto & st : d["streams"].GetArray()) {
		fwt->add_source(Source(st["topic"].GetString(), st["source"].GetString()));
	}
	uint64_t teamid = 0;
	if (master) {
		teamid = master->config.teamid;
	}
	for (auto & d : fwt->demuxers()) {
		for (auto & s : d.sources()) {
			s.teamid = teamid;
		}
	}

	for (auto & d : fwt->demuxers()) {
		LOG(1, "{}", d.to_str());
	}

	{
		Value & nexus_structure = d.FindMember("nexus_structure")->value;
		auto x = fwt->hdf_init(nexus_structure);
		if (x) {
			LOG(7, "ERROR hdf init failed, cancel this write command");
			return;
		}
	}

	if (master) {
		std::string br(d["broker"].GetString());
		auto s = std::unique_ptr< StreamMaster<Streamer, DemuxTopic> >(new StreamMaster<Streamer, DemuxTopic>(br, std::move(fwt)));
		master->stream_masters.push_back(std::move(s));
		s->start();
	}
	g_N_HANDLED += 1;
}

void CommandHandler::handle_exit(rapidjson::Document & d) {
	if (master) master->stop();
}

void CommandHandler::handle(Msg const & msg) {
	using namespace rapidjson;
	auto doc = make_unique<Document>();
	ParseResult err = doc->Parse((char*)msg.data, msg.size);
	if (doc->HasParseError()) {
		LOG(6, "ERROR json parse: {} {}", err.Code(), GetParseError_En(err.Code()));
		throw std::runtime_error("");
	}
	auto & d = * doc;

	uint64_t teamid = 0;
	uint64_t cmd_teamid = 0;
	if (master) {
		teamid = master->config.teamid;
	}
	if (d.HasMember("teamid")) {
		auto & m = d["teamid"];
		if (m.IsInt()) {
			cmd_teamid = d["teamid"].GetUint64();
		}
	}
	if (cmd_teamid != teamid) {
		LOG(1, "INFO command is for teamid {:016x}, we are {:016x}", cmd_teamid, teamid);
		return;
	}

	if (d.HasMember("cmd")) {
		if (d["cmd"].IsString()) {
			std::string cmd(d["cmd"].GetString());
			if (cmd == "FileWriter_new") {
				handle_new(d);
				return;
			}
			if (cmd == "FileWriter_exit") {
				handle_exit(d);
				return;
			}
		}
	}
	{
		auto n1 = std::min(msg.size, (int32_t)1024);
		LOG(3, "ERROR could not figure out this command: {:.{}}", msg.data, n1);
	}
}

}
}
