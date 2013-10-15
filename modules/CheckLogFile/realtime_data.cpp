#include "realtime_data.hpp"

#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>

#include <strEx.h>

#include <nscapi/nscapi_plugin_interface.hpp>

void runtime_data::touch(boost::posix_time::ptime now) {
	BOOST_FOREACH(file_container &fc, files) {
		fc.size = boost::filesystem::file_size(fc.file);
	}
}

bool runtime_data::has_changed() const {
	BOOST_FOREACH(const file_container &fc, files) {
		if (fc.size != boost::filesystem::file_size(fc.file))
			return true;
	}
	return false;
}

void runtime_data::set_files(std::string file_string) {
	if (file_string.empty())
		return;
	files.clear();
	BOOST_FOREACH(const std::string &s, strEx::s::splitEx(file_string, std::string(","))) {
		file_container fc;
		fc.file = s;
		fc.size = boost::filesystem::file_size(fc.file);
		files.push_back(fc);
	}
}

void runtime_data::set_file(std::string file_string) {
	if (file_string.empty())
		return;
	files.clear();
	file_container fc;
	fc.file = file_string;
	fc.size = boost::filesystem::file_size(fc.file);
	files.push_back(fc);
}

bool runtime_data::process_item(filter_type filter) {
	bool matched = false;
	BOOST_FOREACH(file_container &c, files) {
		boost::uintmax_t sz = boost::filesystem::file_size(c.file);
		std::string fname = utf8::cvt<std::string>(c.file);
		std::ifstream file(fname.c_str());
		if (file.is_open()) {
			std::string line;
			if (sz == c.size) {
				continue;
			} else if (sz > c.size) {
				file.seekg(c.size);
			}
			while (file.good()) {
				std::getline(file,line, '\n');
				if (!column_split.empty()) {
					std::list<std::string> chunks = strEx::s::splitEx(line, utf8::cvt<std::string>(column_split));
					boost::shared_ptr<logfile_filter::filter_obj> record(new logfile_filter::filter_obj(fname, line, chunks));
					boost::tuple<bool,bool> ret = filter.match(record);
					if (ret.get<0>()) {
						matched = true;
						if (ret.get<1>()) {
							break;
						}
					}
				}
			}
			file.close();
		} else {
			NSC_LOG_ERROR("Failed to open file: " + fname);
		}
	}
	return matched;
}

void runtime_data::set_split(std::string line, std::string column) {
	if (column.empty())
		column_split = "\t";
	else
		column_split = column;
	strEx::replace(column_split, "\\t", "\t");
	strEx::replace(column_split, "\\n", "\n");
	if (line.empty())
		line = "\n";
	else
		line_split = line;
	strEx::replace(line_split, "\\t", "\t");
	strEx::replace(line_split, "\\n", "\n");
}

