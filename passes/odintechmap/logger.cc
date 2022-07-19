#include "kernel/yosys.h"

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

struct LoganPass : public Pass {
    LoganPass() : Pass("logan", "logger") { }
	void help() override
	{
		log("\n");

		log("\n");
	}

    const std::string str(RTLIL::IdString id)
	{
		std::string str = RTLIL::unescape_id(id);
		for (size_t i = 0; i < str.size(); i++)
			if (str[i] == '#' || str[i] == '=' || str[i] == '<' || str[i] == '>')
				str[i] = '?';
		return str;
	}

	const std::string str(RTLIL::SigBit sig)
	{
		// cstr_bits_seen.insert(sig);

		if (sig.wire == NULL) {
			if (sig == RTLIL::State::S0) return "$false";
			if (sig == RTLIL::State::S1) return "$true";
			return "$undef";
		}

		std::string str = RTLIL::unescape_id(sig.wire->name);
		for (size_t i = 0; i < str.size(); i++)
			if (str[i] == '#' || str[i] == '=' || str[i] == '<' || str[i] == '>')
				str[i] = '?';

		if (sig.wire->width != 1)
			str += stringf("[%d]", sig.wire->upto ? sig.wire->start_offset+sig.wire->width-sig.offset-1 : sig.wire->start_offset+sig.offset);

		return str;
	}

	void execute(std::vector<std::string> args, RTLIL::Design *design) override
	{
        log("hi everyone\n");
        design->sort();
        for (auto cell : design->top_module()->cells()) {
            for (auto &conn : cell->connections()) {
                auto p = cell->getPort(conn.first);
                auto q = conn.second;
                log("%s, %s, %s, %s\n", 
                unescape_id(conn.first).c_str(), 
                conn.second.as_string().c_str(), 
                cell->getPort(conn.first).as_string().c_str(),
                cell->input(conn.first) ? "I" : "O"
                );

                if (conn.second.size() == 1) {
					log(" .%s=%s\n", unescape_id(conn.first).c_str(), str(conn.second[0]).c_str());
					continue;
				}

				Module *m = design->module(cell->type);
				Wire *w = m ? m->wire(conn.first) : nullptr;

				if (w == nullptr) {
					for (int i = 0; i < GetSize(conn.second); i++)
						log(" -%s[%d]=%s\n", unescape_id(conn.first).c_str(), i, str(conn.second[i]).c_str());
				} else {
					for (int i = 0; i < std::min(GetSize(conn.second), GetSize(w)); i++) {
						SigBit sig(w, i);
						log(" +%s[%d]=%s\n", unescape_id(conn.first).c_str(), sig.wire->upto ?
								sig.wire->start_offset+sig.wire->width-sig.offset-1 :
								sig.wire->start_offset+sig.offset, str(conn.second[i]).c_str());
					}
				}
            }
        }
    }
		
} LoganPass;

PRIVATE_NAMESPACE_END