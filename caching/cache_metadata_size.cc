#include "version.h"

#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <iostream>
#include <getopt.h>
#include <stdint.h>
#include <stdexcept>

using namespace boost;
using namespace std;

//----------------------------------------------------------------

namespace {
	struct flags {
		flags()
			: max_hint_width(4) {
		}

		optional<uint64_t> device_size;
		optional<uint32_t> block_size;
		optional<uint64_t> nr_blocks;
		uint32_t max_hint_width;
	};

	void usage(ostream &out, string const &cmd) {
		out << "Usage: " << cmd << " [options]" << endl
		    << "Options:" << endl
		    << "  {-h|--help}" << endl
		    << "  {-V|--version}" << endl
		    << "  {--block-size <sectors>}" << endl
		    << "  {--device-size <sectors>}" << endl
		    << "  {--nr-blocks <natural>}" << endl << endl
		    << "These all relate to the size of the fast device (eg, SSD), rather" << endl
		    << "than the whole cached device." << endl;
	}

	enum parse_result {
		FINISH,
		CONTINUE
	};

	parse_result parse_command_line(string const &prog_name, int argc, char **argv, flags &fs) {

		int c;
		char const short_opts[] = "hV";
		option const long_opts[] = {
			{ "block-size", required_argument, NULL, 0 },
			{ "device-size", required_argument, NULL, 1 },
			{ "nr-blocks", required_argument, NULL, 2 },
			{ "max-hint-width", required_argument, NULL, 3 },
			{ "help", no_argument, NULL, 'h' },
			{ "version", no_argument, NULL, 'V' },
			{ NULL, no_argument, NULL, 0 }
		};

		while ((c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
			switch (c) {
			case 0:
				fs.block_size = lexical_cast<uint32_t>(optarg);
				break;

			case 1:
				fs.device_size = lexical_cast<uint64_t>(optarg);
				break;

			case 2:
				fs.nr_blocks = lexical_cast<uint64_t>(optarg);
				break;

			case 3:
				fs.max_hint_width = lexical_cast<uint32_t>(optarg);
				break;

			case 'h':
				usage(cout, prog_name);
				return FINISH;
				break;

			case 'V':
				cout << THIN_PROVISIONING_TOOLS_VERSION << endl;
				return FINISH;
				break;

			default:
				usage(cerr, prog_name);
				throw runtime_error("Invalid command line");
				break;
			}
		}

		return CONTINUE;
	}

	void expand_flags(flags &fs) {
		if (!fs.device_size && !fs.nr_blocks)
			throw runtime_error("Please specify either --device-size and --block-size, or --nr-blocks.");

		if (fs.device_size && !fs.block_size)
			throw runtime_error("If you specify --device-size you must also give --block-size.");

		if (fs.block_size && !fs.device_size)
			throw runtime_error("If you specify --block-size you must also give --device-size.");

		if (fs.device_size && fs.block_size) {
			uint64_t nr_blocks = *fs.device_size / *fs.block_size;
			if (fs.nr_blocks) {
				if (nr_blocks != *fs.nr_blocks)
					throw runtime_error(
						"Contradictory arguments given, --nr-blocks doesn't match the --device-size and --block-size.");
			} else
				fs.nr_blocks = nr_blocks;
		}
	}

	uint64_t meg(uint64_t n) {
		return n * 2048;
	}

	uint64_t calc_size(flags const &fs) {
		uint64_t const SECTOR_SIZE = 512;
		uint64_t const TRANSACTION_OVERHEAD = meg(4);
		uint64_t const BYTES_PER_BLOCK = 16;
		uint64_t const HINT_OVERHEAD_PER_BLOCK = 8;

		uint64_t mapping_size = (*fs.nr_blocks * BYTES_PER_BLOCK) / SECTOR_SIZE;
		uint64_t hint_size = (*fs.nr_blocks * (fs.max_hint_width + HINT_OVERHEAD_PER_BLOCK)) / SECTOR_SIZE;
		return TRANSACTION_OVERHEAD + mapping_size + hint_size;
	}
}

int main(int argc, char **argv)
{
	flags fs;

	try {
		switch (parse_command_line(argv[0], argc, argv, fs)) {
		case FINISH:
			return 0;

		case CONTINUE:
			break;
		}

		expand_flags(fs);
		cout << calc_size(fs) << " sectors" << endl;

	} catch (std::exception const &e) {
		cerr << e.what();
		return 1;
	}

	return 0;
}

//----------------------------------------------------------------
