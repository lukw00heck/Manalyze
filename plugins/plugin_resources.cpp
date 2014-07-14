/*
    This file is part of Spike Guard.

    Spike Guard is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Spike Guard is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Spike Guard.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <sstream>
#include <algorithm>

#include "yara/yara_wrapper.h"

#include "plugin_framework/plugin_interface.h"
#include "plugin_framework/auto_register.h"

namespace plugin {

class ResourcesPlugin : public IPlugin
{
public:
	int get_api_version() { return 1; }
	
	pString get_id() { 
		return pString(new std::string("resources"));
	}

	pString get_description() { 
		return pString(new std::string("Analyzes the program's resources."));
	}

	pResult analyze(const sg::PE& pe) 
	{
		pResult res(new Result());
		yara::Yara y;

		if (!y.load_rules("yara_rules/magic.yara")) {
			return res;
		}
		
		sg::shared_resources r = pe.get_resources();
		unsigned int size = 0;
		for (sg::shared_resources::element_type::const_iterator it = r->begin() ; it != r->end() ; ++it)
		{
			size += (*it)->get_size();
			yara::const_matches matches = y.scan_bytes(*(*it)->get_raw_data());
			if (matches->size() > 0) 
			{
				std::string ext = matches->at(0)->operator[]("extension");
				if (ext == ".exe" || ext == ".sys" || ext == ".cab")
				{
					res->raise_level(Result::MALICIOUS);
					std::stringstream ss;
					ss << "Resource " << *(*it)->get_name() << " detected as a " << matches->at(0)->operator[]("description") << ".";
					res->add_information(ss.str());
				}
				else if (ext == ".pdf")
				{
					res->raise_level(Result::SUSPICIOUS);
					std::stringstream ss;
					ss << "Resource " << *(*it)->get_name() << " detected as a PDF document.";
					res->add_information(ss.str());
				}
			}
			else 
			{
				if ((*it)->get_entropy() > 7.) 
				{
					std::stringstream ss;
					ss << "Resource " << *(*it)->get_name() << " is possibly compressed or encrypted.";
					res->add_information(ss.str());
				}
			}
		}

		float ratio = (float) size / (float) pe.get_filesize();
		if (ratio > .75) 
		{
			std::stringstream ss;
			ss << "Resources amount for "  << ratio*100 << "% of the executable.";
			res->raise_level(Result::SUSPICIOUS);
			res->add_information(ss.str());
		}

		return res;
	}
};

AutoRegister<ResourcesPlugin> auto_register_resources;

} // !namespace plugin