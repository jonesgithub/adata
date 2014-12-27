#include <vector>
#include <fstream>
#include <ctime>

#include "descrip.h"
#include "util.h"

using namespace std;

namespace csharp_gen
{

  const std::string cpp_lang("csharp");

  inline std::string make_typename(const descrip_define& desc_define, const std::string& name)
  {
    typename_map_type& map_define = get_csharp_typename_map();
    auto find = map_define.find(name);
    if (find == map_define.end())
    {
      if (desc_define.find_decl_type(name))
      {
        return name;
      }
    }
    return find->second;
  }

  std::string make_type_desc(const descrip_define& desc_define, const member_define& define)
  {
    std::string type_name;

    if (define.is_container())
    {
      type_name = make_typename(desc_define, define.m_typename);
      auto find = define.m_options.find(cpp_lang);
      if (find != define.m_options.end())
      {
        type_name = find->second;
      }
      type_name += "<";
      type_name += make_typename(desc_define, define.m_template_parameters[0].m_typename);
      if (define.m_type == e_base_type::map)
      {
        type_name += ",";
        type_name += make_typename(desc_define, define.m_template_parameters[1].m_typename);
      }
      if (desc_define.m_option.m_cpp_allocator.length())
      {
        type_name += ",";
        type_name += desc_define.m_option.m_cpp_allocator;
      }
      type_name += ">";
    }
    else
    {
      type_name = make_typename(desc_define, define.m_typename);
    }
    return type_name;
  }

  std::string make_type_default(const descrip_define&, const member_define& define)
  {
    switch (define.m_type)
    {
    case e_base_type::uint8:
    case e_base_type::int8:
    case e_base_type::uint16:
    case e_base_type::int16:
    case e_base_type::uint32:
    case e_base_type::int32:
    case e_base_type::uint64:
    case e_base_type::int64:
    case e_base_type::float64:
    {
      return define.m_default_value;
    }
    case e_base_type::float32:
    {
      return define.m_default_value + "F";
    }
    case e_base_type::string:
    {
      std::string value = "\"";
      value += define.m_default_value;
      value += "\"";
      return value;
    }
    default:
    {
      break;
    }
    }
    return "";
  }

  void gen_code_type(const descrip_define& desc_define, const type_define& tdefine, std::ofstream& os)
  {
    os << tabs(1) << "public class " << tdefine.m_name << std::endl << "  {" << std::endl;

    std::vector<member_define*> mb_list;

    //sort member by type to arrange layout
    for (auto& member : tdefine.m_members)
    {
      member_define * mb = (member_define *)&member;
      auto it = mb_list.begin();
      for (; it != mb_list.end(); ++it)
      {
        if (member_cmp(mb,*it)<0)
        {
          break;
        }
      }
      mb_list.insert(it, mb);
    }

    for (const auto& mb : mb_list)
    {
      const member_define& member = *mb;
      if (member.m_deleted)
      {
        os << tabs(2) << "//" << member.m_name << " deleted , skip define." << std::endl;
        continue;
      }
      std::string type_name = make_type_desc(desc_define, member);
      os << tabs(2) << "public " << type_name << " " << member.m_name;
      if (member.is_initable())
      {
        os << " = " << make_type_default(desc_define, member);
      }
      else if (member.m_type == e_base_type::string)
      {
        os << " = " << "\"\"";
      }
      else
      {
        os << " = " << "new " << type_name << "()";
      }
      os << ";" << std::endl;
    }

    os << tabs(1) << "}" << std::endl << std::endl;
  }

  inline void gen_trace_error_info(std::ofstream& os, int tab_indent, const std::string& context_name, const std::string& var_name, bool trace_error = true, const std::string sub = "-1")
  {
    if (trace_error == false)
    {
      return;
    }
    os << tabs(tab_indent) << "{if(" << context_name << ".error()){" << context_name << ".trace_error(\"" << var_name << "\"," << sub << ");return;}}";
  }

  std::string gen_csharp_default_value(const member_define& mdefine,const std::string& decl_type)
  {
    switch (mdefine.m_type)
    {
    case e_base_type::uint8:
    case e_base_type::int8:
    case e_base_type::uint16:
    case e_base_type::int16:
    case e_base_type::uint32:
    case e_base_type::int32:
    case e_base_type::uint64:
    case e_base_type::int64:
    {
      return "0";
    }
    case e_base_type::float32:
    {
      return "0.0F";
    }
    case e_base_type::float64:
    {
      return "0.0";
    }
    case e_base_type::string:
    {
      return "\"\"";
    }
    case e_base_type::type:
    {
      std::string value = "new " + decl_type + "()";
      return value;
    }
    default:
    {
      break;
    }
    }
    return "";
  }

  void gen_adata_operator_read_member_code(const descrip_define& desc_define, const type_define& tdefine, const member_define& mdefine, std::ofstream& os, int tab_indent, const std::string& var_name, bool trace_error = true)
  {
    if (mdefine.is_multi())
    {
      if (trace_error)os << std::endl;
      os << tabs(tab_indent) << "{";
      os << std::endl << tabs(tab_indent + 1)
        << "UInt32 len" << tab_indent << " = stream.check_read_size(";
      if (mdefine.m_size.length())
      {
        os << mdefine.m_size;
      }
      os << ");";
      os << std::endl;
      gen_trace_error_info(os, tab_indent + 1, "stream", mdefine.m_name, trace_error);
      if (trace_error)os << std::endl;
      if (mdefine.m_type == e_base_type::string)
      {
        os << tabs(tab_indent + 1) << "stream.stream_read(ref " << var_name << ",len" << tab_indent << ");";
        if (trace_error)os << std::endl;
        gen_trace_error_info(os, tab_indent + 1, "stream", mdefine.m_name, trace_error);
      }
      else if (mdefine.m_type == e_base_type::list)
      {
        os << tabs(tab_indent + 1) << "for (int i = 0 ; i < len" << tab_indent << " ; ++i)" << std::endl;
        os << tabs(tab_indent + 1) << "{" << std::endl;
        std::string decl_type = make_type_desc(desc_define, mdefine.m_template_parameters[0]);
        os << tabs(tab_indent + 2) << decl_type << " element";
        std::string decl_value = gen_csharp_default_value(mdefine.m_template_parameters[0], decl_type);
        if(!decl_value.empty())
        {
          os << "= " << decl_value;
        }
        os << "; " << std::endl;
        gen_adata_operator_read_member_code(desc_define, tdefine, mdefine.m_template_parameters[0], os, tab_indent + 2, "element", false);
        os << tabs(tab_indent + 2) << var_name << ".Add(element);" << std::endl;
        gen_trace_error_info(os, tab_indent + 2, "stream", mdefine.m_name, true, "(int)i");
        os << std::endl;
        os << tabs(tab_indent + 1) << "}";
      }
      else if (mdefine.m_type == e_base_type::map)
      {
        os << tabs(tab_indent + 1) << "for (int i = 0 ; i < len" << tab_indent << " ; ++i)" << std::endl;
        os << tabs(tab_indent + 1) << "{" << std::endl;
        std::string decl_type1 = make_type_desc(desc_define, mdefine.m_template_parameters[0]);
        os << tabs(tab_indent + 2) << decl_type1 << " first_element";
        std::string decl_value1 = gen_csharp_default_value(mdefine.m_template_parameters[0], decl_type1);
        if (!decl_value1.empty())
        {
          os << "= " << decl_value1;
        }
        os << "; " << std::endl;
        std::string decl_type2 = make_type_desc(desc_define, mdefine.m_template_parameters[1]);
        os << tabs(tab_indent + 2) << decl_type2 << " second_element";
        std::string decl_value2 = gen_csharp_default_value(mdefine.m_template_parameters[1], decl_type2);
        if (!decl_value2.empty())
        {
          os << "= " << decl_value2;
        }
        os << "; " << std::endl;

        gen_adata_operator_read_member_code(desc_define, tdefine, mdefine.m_template_parameters[0], os, tab_indent + 2, "first_element", false);
        gen_adata_operator_read_member_code(desc_define, tdefine, mdefine.m_template_parameters[1], os, tab_indent + 2, "second_element", false);
        gen_trace_error_info(os, tab_indent + 2, "stream", mdefine.m_name, true, "(int)i");
        os << std::endl;
        os << tabs(tab_indent + 2) << var_name << ".Add(first_element,second_element);" << std::endl;
        os << tabs(tab_indent + 1) << "}";
      }
      os << std::endl << tabs(tab_indent) << "}" << std::endl;
    }
    else
    {
      os << tabs(tab_indent) << "{";
      os << "stream.";
      if (mdefine.m_fixed)
      {
        os << "fix_";
      }
      os << "stream_read(ref " << var_name << ");";
      gen_trace_error_info(os, 0, "stream", mdefine.m_name, trace_error);
      os << "}" << std::endl;
    }
  }

  void gen_adata_operator_read_skip_member_code(const descrip_define& desc_define, const type_define& tdefine, const member_define& mdefine, std::ofstream& os, int tab_indent, const std::string&, bool trace_error = true)
  {
    if (mdefine.is_multi())
    {
      if (trace_error)os << std::endl;
      os << tabs(tab_indent) << "{";
      os << std::endl << tabs(tab_indent + 1)
        << "UInt32 len" << tab_indent << " = stream.check_read_size(";
      if (mdefine.m_size.length())
      {
        os << mdefine.m_size;
      }
      os << ");";
      os << std::endl;
      gen_trace_error_info(os, tab_indent + 1, "stream", mdefine.m_name, trace_error);
      if (trace_error)os << std::endl;
      if (mdefine.m_type == e_base_type::string)
      {
        os << tabs(tab_indent + 1) << "stream.skip_read_byte(len" << tab_indent << ");" << std::endl;
        gen_trace_error_info(os, tab_indent + 1, "stream", mdefine.m_name, trace_error);
        if (trace_error)os << std::endl;
      }
      else if (mdefine.m_type == e_base_type::list)
      {
        os << tabs(tab_indent + 1) << "for (int i = 0 ; i < len" << tab_indent << " ; ++i)" << std::endl;
        os << tabs(tab_indent + 1) << "{" << std::endl;
        gen_adata_operator_read_skip_member_code(desc_define, tdefine, mdefine.m_template_parameters[0], os, tab_indent + 2, "element", false);
        gen_trace_error_info(os, tab_indent + 2, "stream", mdefine.m_name, true, "(int)i");
        os << std::endl;
        os << tabs(tab_indent + 1) << "}" << std::endl;
      }
      else if (mdefine.m_type == e_base_type::map)
      {
        os << tabs(tab_indent + 1) << "for (int i = 0 ; i < len" << tab_indent << " ; ++i)" << std::endl;
        os << tabs(tab_indent + 1) << "{" << std::endl;
        gen_adata_operator_read_skip_member_code(desc_define, tdefine, mdefine.m_template_parameters[0], os, tab_indent + 2, "first_element", false);
        gen_adata_operator_read_skip_member_code(desc_define, tdefine, mdefine.m_template_parameters[1], os, tab_indent + 2, "second_element", false);
        gen_trace_error_info(os, tab_indent + 2, "stream", mdefine.m_name, true, "(int)i");
        os << std::endl;
        os << tabs(tab_indent + 1) << "}" << std::endl;
      }
      os << tabs(tab_indent) << "}" << std::endl;
    }
    else
    {
      os << tabs(tab_indent) << "{" << make_type_desc(desc_define, mdefine) << " dummy_value = ";
      if(mdefine.is_initable())
      {
        os << "0";
      }
      else
      {
        os << "null";
      }
      os << ";";
      os << "stream.";
      if (mdefine.m_fixed)
      {
        os << "fix_";
      }
      os << "skip_read(dummy_value);";
      gen_trace_error_info(os, 0, "stream", mdefine.m_name, trace_error);
      os << "}" << std::endl;
    }
  }

  inline void gen_adata_read_member_code(const descrip_define& desc_define, const type_define& tdefine, const member_define& mdefine, std::ofstream& os, int tab_indent)
  {
    std::string var_name = "value.";
    var_name += mdefine.m_name;

    if (mdefine.m_deleted)
    {
      gen_adata_operator_read_skip_member_code(desc_define, tdefine, mdefine, os, tab_indent, var_name);
    }
    else
    {
      gen_adata_operator_read_member_code(desc_define, tdefine, mdefine, os, tab_indent, var_name);
    }
  }

  void gen_adata_operator_read_type_code(const descrip_define& desc_define, const type_define& tdefine, std::ofstream& os)
  {
    std::string full_type_name = tdefine.m_name;
    os << tabs(2) << "static public void stream_read(this zero_copy_buffer stream, ref " << full_type_name << " value)" << std::endl;
    os << tabs(2) << "{" << std::endl;
    os << tabs(3) << "UInt64 tag = 0;" << std::endl;
    os << tabs(3) << "stream.stream_read(ref tag);" << std::endl;
    os << tabs(3) << "if(stream.error()){return;}" << std::endl;
    uint64_t tag_mask = 1;
    uint64_t total_mask = 0;
    for (const auto& member : tdefine.m_members)
    {
      os << tabs(3) << "if((tag&" << tag_mask << ")>0)";
      gen_adata_read_member_code(desc_define, tdefine, member, os, 3);
      total_mask |= tag_mask;
      tag_mask <<= 1;
    }
    os << tabs(3) << "if((tag&(~(UInt64)" << total_mask << "))>0){stream.error_code = (error_code_t.undefined_member_protocol_not_compatible);return;}" << std::endl;
    os << tabs(2) << "}" << std::endl << std::endl;
  }

  inline void gen_adata_skip_read_member_code(const descrip_define& desc_define, const type_define& tdefine, const member_define& mdefine, std::ofstream& os, int tab_indent)
  {
    std::string var_name = "value.";
    var_name += mdefine.m_name;
    gen_adata_operator_read_skip_member_code(desc_define, tdefine, mdefine, os, tab_indent, var_name);
  }

  void gen_adata_operator_skip_read_type_code(const descrip_define& desc_define, const type_define& tdefine, std::ofstream& os)
  {
    std::string full_type_name = tdefine.m_name;
    os << tabs(2) << "static public void skip_read(this zero_copy_buffer stream, " << full_type_name << " value)" << std::endl;
    os << tabs(2) << "{" << std::endl;
    os << tabs(3) << "UInt64 tag = 0;" << std::endl;
    os << tabs(3) << "stream.stream_read(ref tag);" << std::endl;
    os << tabs(3) << "if(stream.error()){ return; }" << std::endl;
    uint64_t tag_mask = 1;
    uint64_t total_mask = 0;
    for (const auto& member : tdefine.m_members)
    {
      os << tabs(3) << "if((tag&" << tag_mask << ")>0)";
      gen_adata_skip_read_member_code(desc_define, tdefine, member, os, 3);
      total_mask |= tag_mask;
      tag_mask <<= 1;
    }
    os << tabs(3) << "if((tag&(~(UInt64)" << total_mask << "))>0){stream.error_code = (error_code_t.undefined_member_protocol_not_compatible);return;}" << std::endl;
    os << tabs(2) << "}" << std::endl << std::endl;
  }

  inline void gen_size_check_write_member_code(const descrip_define&, const type_define&, const member_define& mdefine, std::ofstream& os, int tab_indent, bool trace_error = true)
  {
    os << tabs(tab_indent) << "if(len" << tab_indent << ">" << mdefine.m_size
      << "){ stream.error_code = (error_code_t.number_of_element_not_macth);}";
    if (trace_error)os << std::endl;
    gen_trace_error_info(os, tab_indent, "stream", mdefine.m_name, trace_error);
    if (trace_error)os << std::endl;
  }

  void gen_adata_operator_write_tag_code(const descrip_define&, const type_define& tdefine, std::ofstream& os, int tab_indent)
  {
    uint64_t tag = 0;
    uint64_t tag_mask = 1;
    for (const auto& member : tdefine.m_members)
    {
      if (!member.is_multi() && (!member.m_deleted))
      {
        tag |= tag_mask;
      }
      tag_mask <<= 1;
    }
    tag_mask = 1;
    os << tabs(tab_indent) << "UInt64 tag = " << tag << ";" << std::endl;
    for (const auto& member : tdefine.m_members)
    {
      if (member.m_deleted == 0)
      {
        if (member.is_container())
        {
          os << tabs(tab_indent) << "if(value." << member.m_name << ".Count > 0){tag|=" << tag_mask << ";}" << std::endl;
        }
        else if(member.m_type == e_base_type::string)
        {
          os << tabs(tab_indent) << "if(value." << member.m_name << ".Length > 0){tag|=" << tag_mask << ";}" << std::endl;
        }
      }
      tag_mask <<= 1;
    }
  }

  void gen_member_pre_write_type_code(const descrip_define& desc_define, const type_define& tdefine, const member_define& mdefine, std::ofstream& os, int tab_indent, const std::string& var_name, bool trace_error = true)
  {
    if (mdefine.is_multi())
    {
      if (mdefine.m_type == e_base_type::string)
      {
        os << tabs(tab_indent) << "UInt32 len" << tab_indent << " = (UInt32)" << var_name << ".Length;" << std::endl;
        if (mdefine.m_size.length())
        {
          gen_size_check_write_member_code(desc_define, tdefine, mdefine, os, tab_indent, trace_error);
        }
        os << tabs(tab_indent) << "stream.pre_write(len" << tab_indent << ");" << std::endl;
        os << tabs(tab_indent) << "stream.write_bytes += (int)len" << tab_indent << ";";
        os << std::endl;
      }
      else if (mdefine.m_type == e_base_type::list)
      {
        os << tabs(tab_indent) << "UInt32 len" << tab_indent << " = (UInt32)" << var_name << ".Count;" << std::endl;
        if (mdefine.m_size.length())
        {
          gen_size_check_write_member_code(desc_define, tdefine, mdefine, os, tab_indent, trace_error);
        }
        os << tabs(tab_indent) << "stream.pre_write(len" << tab_indent << ");" << std::endl;
        os << tabs(tab_indent) << "Int32 count = 0;" << std::endl;
        std::string decl_type = make_type_desc(desc_define, mdefine.m_template_parameters[0]);
        os << tabs(tab_indent) << "foreach (" << decl_type << " i in " << var_name << ")" << std::endl;
        os << tabs(tab_indent) << "{" << std::endl;
        gen_member_pre_write_type_code(desc_define, tdefine, mdefine.m_template_parameters[0], os, tab_indent + 1, "i", false);
        gen_trace_error_info(os, tab_indent + 1, "stream", mdefine.m_name, true, "count");
        os << std::endl;
        os << tabs(tab_indent + 1) << "++count;" << std::endl;
        os << tabs(tab_indent) << "}";
        os << std::endl;
      }
      else if (mdefine.m_type == e_base_type::map)
      {
        os << tabs(tab_indent) << "UInt32 len" << tab_indent << " = (UInt32)" << var_name << ".Count;" << std::endl;
        if (mdefine.m_size.length())
        {
          gen_size_check_write_member_code(desc_define, tdefine, mdefine, os, tab_indent, trace_error);
        }
        os << tabs(tab_indent) << "stream.pre_write(len" << tab_indent << ");" << std::endl;
        os << tabs(tab_indent) << "Int32 count = 0;" << std::endl;
        os << tabs(tab_indent) << "foreach (var i in " << var_name << ")" << std::endl;
        os << tabs(tab_indent) << "{" << std::endl;
        gen_member_pre_write_type_code(desc_define, tdefine, mdefine.m_template_parameters[0], os, tab_indent + 1, "i.Key", false);
        gen_member_pre_write_type_code(desc_define, tdefine, mdefine.m_template_parameters[1], os, tab_indent + 1, "i.Value", false);
        gen_trace_error_info(os, tab_indent + 1, "stream", mdefine.m_name, true, "count");
        os << std::endl;
        os << tabs(tab_indent + 1) << "++count;" << std::endl;
        os << tabs(tab_indent) << "}";
        os << std::endl;
      }
    }
    else
    {
      os << tabs(tab_indent);
      os << "stream.";
      if (mdefine.m_fixed)
      {
        os << "fix_";
      }
      os << "pre_write(" << var_name << ");" << std::endl;
      gen_trace_error_info(os, tab_indent, "stream", mdefine.m_name, trace_error);
      if (trace_error)os << std::endl;
    }
  }

  void gen_adata_operator_pre_write_type_code(const descrip_define& desc_define, const type_define& tdefine, std::ofstream& os)
  {
    std::string full_type_name = tdefine.m_name;
    os << tabs(2) << "public static void pre_write(this zero_copy_buffer stream , " << full_type_name << " value)" << std::endl;
    os << tabs(2) << "{" << std::endl;
    gen_adata_operator_write_tag_code(desc_define, tdefine, os, 3);
    uint64_t tag_mask = 1;
    for (const auto& member : tdefine.m_members)
    {
      std::string var_name = "value.";
      var_name += member.m_name;
      if (!member.m_deleted)
      {
        if (member.is_multi())
        {
          os << tabs(3) << "if((tag&" << tag_mask << ")>0)" << std::endl;
        }
        os << tabs(3) << "{" << std::endl;
        gen_member_pre_write_type_code(desc_define, tdefine, member, os, 4, var_name);
        os << tabs(3) << "}" << std::endl;
      }
      else
      {
        os << tabs(3) << "//" << var_name << " deleted , skip write." << std::endl;
      }
      tag_mask <<= 1;
    }
    os << tabs(3) << "stream.pre_write(tag);" << std::endl;
    os << tabs(3) << "return;" << std::endl;
    os << tabs(2) << "}" << std::endl << std::endl;
  }

  void gen_member_write_type_code(const descrip_define& desc_define, const type_define& tdefine, const member_define& mdefine, std::ofstream& os, int tab_indent, const std::string& var_name, bool trace_error = true)
  {
    if (mdefine.is_multi())
    {
      if (trace_error)os << std::endl;
      os << tabs(tab_indent) << "{";
      os << std::endl;

      if (mdefine.m_type == e_base_type::string)
      {
        os << tabs(tab_indent + 1) << "UInt32 len" << tab_indent << " = (UInt32)" << var_name << ".Length;" << std::endl;
        os << tabs(tab_indent + 1) << "stream.stream_write(len" << tab_indent << ");" << std::endl;
        os << tabs(tab_indent + 1) << "stream.stream_write(" << var_name << ",len" << tab_indent << ");" << std::endl;
        gen_trace_error_info(os, tab_indent + 1, "stream", mdefine.m_name, trace_error);
        if (trace_error)os << std::endl;
      }
      else if (mdefine.m_type == e_base_type::list)
      {
        os << tabs(tab_indent + 1) << "UInt32 len" << tab_indent << " = (UInt32)" << var_name << ".Count;" << std::endl;
        os << tabs(tab_indent + 1) << "stream.stream_write(len" << tab_indent << ");" << std::endl;
        os << tabs(tab_indent + 1) << "int count = 0;" << std::endl;
        std::string decl_type = make_type_desc(desc_define, mdefine.m_template_parameters[0]);
        os << tabs(tab_indent + 1) << "foreach (" << decl_type << " i in " << var_name << ")" << std::endl;
        os << tabs(tab_indent + 1) << "{" << std::endl;
        gen_member_write_type_code(desc_define, tdefine, mdefine.m_template_parameters[0], os, tab_indent + 2, "i", false);
        os << std::endl;
        gen_trace_error_info(os, tab_indent + 2, "stream", mdefine.m_name, true, "count");
        os << std::endl;
        os << tabs(tab_indent + 2) << "++count;" << std::endl;
        os << tabs(tab_indent + 1) << "}" << std::endl;
      }
      else if (mdefine.m_type == e_base_type::map)
      {
        os << tabs(tab_indent + 1) << "UInt32 len" << tab_indent << " = (UInt32)" << var_name << ".Count;" << std::endl;
        os << tabs(tab_indent + 1) << "stream.stream_write(len" << tab_indent << ");" << std::endl;
        os << tabs(tab_indent + 1) << "int count = 0;" << std::endl;
        os << tabs(tab_indent + 1) << "foreach (var i in " << var_name << ")" << std::endl;
        os << tabs(tab_indent + 1) << "{" << std::endl;
        gen_member_write_type_code(desc_define, tdefine, mdefine.m_template_parameters[0], os, tab_indent + 2, "i.Key", false);
        os << std::endl;
        gen_member_write_type_code(desc_define, tdefine, mdefine.m_template_parameters[1], os, tab_indent + 2, "i.Value", false);
        os << std::endl;
        gen_trace_error_info(os, tab_indent + 2, "stream", mdefine.m_name, true, "count");
        os << std::endl;
        os << tabs(tab_indent + 2) << "++count;" << std::endl;
        os << tabs(tab_indent + 1) << "}" << std::endl;
      }
      os << tabs(tab_indent) << "}";
    }
    else
    {
      os << tabs(tab_indent) << "{";
      os << "stream.";
      if (mdefine.m_fixed)
      {
        os << "fix_";
      }
      os << "stream_write(" << var_name << ");";
      gen_trace_error_info(os, 0, "stream", mdefine.m_name, trace_error);
      os << "}";
    }
  }

  void gen_adata_operator_write_type_code(const descrip_define& desc_define, const type_define& tdefine, std::ofstream& os)
  {
    std::string full_type_name = tdefine.m_name;
    os << tabs(2) << "static public void stream_write(this zero_copy_buffer stream , " << full_type_name << " value)" << std::endl;
    os << tabs(2) << "{" << std::endl;
    gen_adata_operator_write_tag_code(desc_define, tdefine, os, 3);
    os << tabs(3) << "stream.stream_write(tag);" << std::endl;
    uint64_t tag_mask = 1;
    uint64_t total_mask = 0;
    for (const auto& member : tdefine.m_members)
    {
      std::string var_name = "value.";
      var_name += member.m_name;
      if (!member.m_deleted)
      {
        if (member.is_multi())
        {
          os << tabs(2) << "if((tag&" << tag_mask << ")>0)";
        }
        gen_member_write_type_code(desc_define, tdefine, member, os, 3, var_name);
        os << std::endl;
      }
      else
      {
        os << tabs(2) << "//" << var_name << " deleted , skip write." << std::endl;
      }
      total_mask |= tag_mask;
      tag_mask <<= 1;
    }
    os << tabs(3) << "return;" << std::endl;
    os << tabs(2) << "}" << std::endl << std::endl;

  }

  inline void gen_adata_operator_type_code(const descrip_define& desc_define, const type_define& tdefine, std::ofstream& os)
  {
    gen_adata_operator_read_type_code(desc_define, tdefine, os);
    gen_adata_operator_skip_read_type_code(desc_define, tdefine, os);
    gen_adata_operator_pre_write_type_code(desc_define, tdefine, os);
    gen_adata_operator_write_type_code(desc_define, tdefine, os);
  }

const char * op_code_define = R"(
  static class stream
  {

)";

  void gen_adata_operator_code(const descrip_define& desc_define, std::ofstream& os)
  {
    os << op_code_define;

    for (auto& t_define : desc_define.m_types)
    {
      gen_adata_operator_type_code(desc_define, t_define, os);
    }
    os << tabs(1) << "}" << std::endl << std::endl;
    os << std::endl;
  }

  void gen_type_code(const descrip_define& desc_define, std::ofstream& os)
  {
    for (const auto& name : desc_define.m_namespace.m_names)
    {
      os << "namespace " << name << " {";
    }
    os << std::endl;

    for (auto& t_define : desc_define.m_types)
    {
      gen_code_type(desc_define, t_define, os);
    }

  }

const char * using_define = R"(using System;
using System.IO;
using System.Collections.Generic;
using adata;

)";

  void	gen_code(const descrip_define& define, const std::string& cpp_file)
  {
    ofstream os(cpp_file);

    os << using_define;

    gen_type_code(define, os);

    gen_adata_operator_code(define, os);

    for (const auto& name : define.m_namespace.m_names)
    {
      os << "}";
    }
    os << std::endl << std::endl;

    os.close();
  }

}