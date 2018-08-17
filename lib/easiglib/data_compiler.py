#!/usr/bin/env python

import numpy

class_name = "Data"
file_name = "data"

def type_of(value):
    return {
        int: lambda: "s32",
        numpy.int32: lambda: "s32",
        numpy.int16: lambda: "s16",
        bool: lambda: "bool",
        float: lambda: "f",
        numpy.float64: lambda: "f",
        str: lambda: "string",
        tuple: lambda: ("tuple<%s>" % (", ".join(map(type_of, value)))),
        list: lambda: ("Buffer<%s, %d>" % (type_of(value[0])), len(value)),
        numpy.ndarray: lambda: ("Buffer<%s, %d>" % (type_of(value[0]), len(value))),
    }[type(value)]()

def is_base_type(value):
    t = type(value)
    if (t is int or
        t is bool or
        t is numpy.int16 or
        t is numpy.int32 or
        t is numpy.float64 or
        t is numpy.float32):
        return True;
    else:
        return False;

def write_value_of(file, value, indent_level):
    t = type(value)
    if (t is int or t is numpy.int16): file.write("%d_s16" % (value))
    elif (t is bool): file.write("true" if value else "false")
    elif (t is float or t is numpy.float64): file.write("%f_f" % (value))
    elif (t is str): file.write("\"%s\"" % (value))
    elif (t is tuple):
        file.write("{")
        for v in value:
            write_value_of(file, v, indent_level+1)
            file.write(",")
        file.write("}")
    elif (t is list or t is numpy.ndarray):
        file.write("{{\n")
        for v in value:
            file.write(" " * indent_level * 2)
            write_value_of(file, v, indent_level+1)
            file.write(",\n")
        file.write(" " * (indent_level-1) * 2)
        file.write("}}")
    else: raise Exception("unknown type " + str(t))

def write_definition(file, name, value):
    file.write("const ");
    file.write(type_of(value))
    file.write(" " + name + " = ")
    write_value_of(file, value, 1)
    file.write(";\n")

def write_implementation_file(file, data):
    file.write("#include \"data.hh\"\n\n")
    file.write("using namespace std;\n\n")
    for name, value in data.items():
        if (not is_base_type(value)):
            file.write("/* %s */\n" % (name))
            write_definition(file, class_name+"::"+name, value)
            file.write("\n")

def write_header_file(file, data):
    file.write("#include \"numtypes.hh\"\n")
    file.write("#include \"buffer.hh\"\n\n")
    file.write("#pragma once\n\n")
    file.write("using namespace std;\n\n")
    file.write("struct Data {\n")
    for name, value in data.items():
        if (is_base_type(value)):
            file.write("  static constexpr %s %s = " % (type_of(value), name))
            write_value_of(file, value, 0)
            file.write(";\n")
        else:
            file.write("  static const %s %s;\n" % (type_of(value), name))
    file.write("};")

def compile(data):
    with open(file_name+".hh", "w") as header_file:
        write_header_file(header_file, data)
    with open(file_name+".cc", "w") as implem_file:
        write_implementation_file(implem_file, data)
