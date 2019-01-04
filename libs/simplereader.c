//
// Created by WangJingjin on 2018-12-12.
//
#include "simplereader.h"

void get_die_and_siblings(Dwarf_Debug dbg, Dwarf_Die in_die,int in_level) {
    int res = DW_DLV_ERROR;
    Dwarf_Die cur_die=in_die;
    Dwarf_Die sib_die=in_die;
    Dwarf_Die child = 0;
    Dwarf_Error error;

    /* Who am I? */
    print_die_data(dbg,in_die,in_level);

    /* First son, if any */
    res = dwarf_child(cur_die,&child,&error);


    /* traverse tree depth first */

    if(res == DW_DLV_OK)
    { get_die_and_siblings(dbg, child, in_level+1); /* recur on the first son */
        sib_die = child;
        while(res == DW_DLV_OK) {
            cur_die = sib_die;
            res = dwarf_siblingof(dbg, cur_die, &sib_die, &error);
            get_die_and_siblings(dbg, sib_die, in_level+1); /* recur others */
        };
    }

    return;
}

void print_die_data(Dwarf_Debug dbg, Dwarf_Die print_me,int level) {
    char *name = 0;
    Dwarf_Error error = 0;
    Dwarf_Half tag = 0;
    const char *tagname = 0;
    Dwarf_Line* line;

    Dwarf_Bool bAttr;
    Dwarf_Attribute attr;
    int res = 0;
    Dwarf_Unsigned in_line;
    Dwarf_Unsigned in_file = 0;

    Dwarf_Locdesc * loc_list;
    Dwarf_Signed num_loc;

    Dwarf_Off  ptr_address = 0;

    int has_line_data = !dwarf_hasattr( print_me, DW_AT_decl_line, &bAttr, &error) && bAttr;
    if(has_line_data){

        /* Here we know that we have debug information for line numbers
           in this compilation unit. Let's keep working */

        /* Using short-circuiting to ensure all steps are done in order; if a chain finishes, we know we have stored our values */
        int got_name = !dwarf_diename(print_me,&name,&error);
        int got_line = !dwarf_attr(print_me, DW_AT_decl_line, &attr, &error) && !dwarf_formudata(attr, &in_line, &error);
        int got_file = !dwarf_attr(print_me, DW_AT_decl_file, &attr, &error) && !dwarf_formudata(attr, &in_file, &error);
        int got_loclist = !dwarf_hasattr(print_me, DW_AT_location, &bAttr, &error) && !dwarf_attr(print_me, DW_AT_location, &attr, &error)
                          && !dwarf_loclist(attr, &loc_list, &num_loc, &error);

        int got_tag_name = !dwarf_tag(print_me,&tag,&error) && dwarf_get_TAG_name(tag,&tagname);

        if(got_name && got_line && got_file){

            /* Location lists are structs; see ftp://ftp.sgi.com/sgi/dwarf/libdwarf.h */
            if(got_loclist && loc_list[0].ld_cents == 1){
                printf("<%llu:%llu> tag: %d %s  name: %s loc: %lld\n",in_file, in_line,tag,tagname,name,loc_list[0].ld_s[0].lr_number);
            } else if (tag == 46) {
                !dwarf_hasattr( print_me, DW_AT_object_pointer, &bAttr, &error) &&
                bAttr && !dwarf_attr(print_me, DW_AT_object_pointer, &attr, &error)
                && !dwarf_formref(attr, &ptr_address, &error)
                && printf("<%llu:%llu> tag: %d %s  name: %s obj_pointer: 0x%llx \n",in_file, in_line,tag,tagname,name, ptr_address);

            }

        }

    }
    dwarf_dealloc(dbg,name,DW_DLA_STRING);
}

void read_cu_list(Dwarf_Debug dbg) {
    Dwarf_Unsigned cu_header_length = 0;
    Dwarf_Half version_stamp = 0;
    Dwarf_Unsigned abbrev_offset = 0;
    Dwarf_Half address_size = 0;
    Dwarf_Unsigned next_cu_header = 0;
    Dwarf_Error error;
    int cu_number = 0;

    for(;;++cu_number) {
        Dwarf_Die no_die = 0;
        Dwarf_Die cu_die = 0;
        int res = DW_DLV_ERROR;
        res = dwarf_next_cu_header(dbg,&cu_header_length,
                                   &version_stamp, &abbrev_offset, &address_size,
                                   &next_cu_header, &error);

        if(res == DW_DLV_ERROR) {
            printf("Error in dwarf_next_cu_header\n");
            exit(1);
        }
        if(res == DW_DLV_NO_ENTRY) {
            printf("DONE\n");
            /* Done. */
            return;
        }
        /* The CU will have a single sibling, a cu_die. */
        res = dwarf_siblingof(dbg,no_die,&cu_die,&error);
        if(res == DW_DLV_ERROR) {
            printf("Error in dwarf_siblingof on CU die \n");
            exit(1);
        }
        if(res == DW_DLV_NO_ENTRY) {
            /* Impossible case. */
            printf("no entry! in dwarf_siblingof on CU die \n");
            exit(1);
        }

        get_die_and_siblings(dbg,cu_die,0);

        dwarf_dealloc(dbg,cu_die,DW_DLA_DIE);

    }
}
