#include <iostream>
#include <string>

#include <getopt.h>
#include <unistd.h>

#include "config.h"
#include "shim.h"

using namespace project;
using namespace std;


void load_from_cfg(const string &cfg)
{
    // load object config if cfg file provided
    cout << "loading " << cfg << " ..." << endl;
    shim_cfg c;
    if (c.load_config(cfg))
    {
        if (c.parse_config())
        {
            cout << "sucessfully loaded " << cfg << endl;
            // c.build_config();
            //     c.save_config("cfg/output.cfg");
            //     cout << "successfully built config" << endl;
            int dst_ver = enum_ver_2;
            if (c.migrate_config(dst_ver))
            {
                cout << "successfully migrated config to version " << dst_ver << endl;

                if (c.build_config()) {
                    c.save_config("cfg/output.cfg");
                    cout << "successfully builded " << cfg << endl;
                }
            }
        }
    }
    if (!c.get_error().empty())
        cerr << "failed to load/parse/migrate/build " << cfg << ", " << c.get_error() << endl;
}

void read_cfg_meta(const string &cfg)
{
    // load object config if cfg file provided
    cout << "loading " << cfg << " ..." << endl;
    meta_cfg m;
    if (m.load_config(cfg))
    {
        if (m.parse_config())
            cout << "sucessfully loaded " << cfg << endl;
    }
    if (!m.get_error().empty())
        cerr << "failed to load/parse " << cfg << ", " << m.get_error() << endl;
}

int main(int argc, char *argv[])
{
    enum op_t
    {
        enum_op_meta,
        enum_op_default
    };
    op_t op = enum_op_default;
    string obj_cfg;

    // parse cmd line arguments
    static struct option options[] =
    {
        { "meta", no_argument, 0, 'm' },
        { "incfg", required_argument, 0, 'i' },
        { 0, 0, 0, 0 }
    };

    int opt = 0, idx = 0;
    while ((opt = getopt_long(argc, argv, "mi:", options, &idx)) != -1)
    {
        switch (opt)
        {
            case 'm':
                op = enum_op_meta;
                break;
            case 'i':
                obj_cfg = optarg;
                break;
            default:
                cerr << "unknown argument" << endl;
                break;
        }
    }

    if (op == enum_op_default)
    {
        if (obj_cfg.empty())
            cerr << "no cfg file" << endl;
        else
            load_from_cfg(obj_cfg);

        // dump shim
        // cout << endl << "shim memory store: " << endl;
        // shim::instance().dump();
    }
    else if (op == enum_op_meta)
    {
        if (!obj_cfg.empty())
            read_cfg_meta(obj_cfg);
        else
            cerr << "no cfg file" << endl;
    }

    cout << "done." << endl;
    return 0;
}
