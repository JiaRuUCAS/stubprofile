/*
 * See the dyninst/COPYRIGHT file for copyright information.
 * 
 * We provide the Paradyn Tools (below described as "Paradyn")
 * on an AS IS basis, and do not warrant its validity or performance.
 * We reserve the right to update, modify, or discontinue this
 * software at any time.  We shall have no obligation to supply such
 * updates or modifications or any other form of support to you.
 * 
 * By your use of Paradyn, you understand and agree that we (or any
 * other person or entity with proprietary rights in Paradyn) are
 * under no obligation to provide either maintenance services,
 * update services, notices of latent defects, or correction of
 * defects for Paradyn.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "common/serialize.h"

#if !defined(SERIALIZATION_DISABLED)

#include "common/src/pathName.h"
#include <dlfcn.h>

#define XMLCHAR_CAST (const char *)


namespace Dyninst {

bool start_xml_elem(void * /*writer*/, const char * /*xmlChar*/)
{
   fprintf(stderr, "%s[%d]:  xml output is disabled\n", FILE__, __LINE__);
   return false;
}

bool end_xml_elem(void * /*writer*/)
{
   fprintf(stderr, "%s[%d]:  xml output is disabled\n", FILE__, __LINE__);
   return false;
}

bool write_xml_elem(void * /*writer*/, const char * /*tag*/, const char * /*fmt*/, ...)
{
   fprintf(stderr, "%s[%d]:  xml output is disabled\n", FILE__, __LINE__);
   return false;
}
}

namespace Dyninst {
bool start_xml_elem(SerDesXML &s, const char *tag)
{
	return start_xml_elem(s.writer, tag);
}
bool end_xml_elem(SerDesXML &s)
{
	return end_xml_elem(s.writer);
}
}

namespace Dyninst {
bool ifxml_start_element(SerializerBase *sb, const char *tag)
{
   if (!sb->isXML())
	   return false;
   if (!sb->isOutput())
   {
      fprintf(stderr, "%s[%d]:  ERROR:  request to deserialize xml\n", FILE__, __LINE__);
      return false;
   }

   SerDes &sd = sb->getSD();
   SerDesXML *sdxml = dynamic_cast<SerDesXML *>(&sd);
   assert(sdxml);
   ::start_xml_elem(sdxml->writer, tag);

   return true;
}
}

namespace Dyninst {
COMMON_EXPORT bool ifxml_end_element(SerializerBase *sb, const char * /*tag*/)
{
   if (!sb->isXML())
	   return false;
   if (!sb->isOutput())
   {
      fprintf(stderr, "%s[%d]:  ERROR:  request to deserialize xml\n", FILE__, __LINE__);
      return false;
   }

   SerDes &sd = sb->getSD();
   SerDesXML *sdxml = dynamic_cast<SerDesXML *>(&sd);
   assert(sdxml);
   ::end_xml_elem(sdxml->writer);
   
   return true;
}
}

bool SerializerXML::start_xml_element(SerializerBase *sb, const char *tag)
{
	SerializerXML *sxml = dynamic_cast<SerializerXML *>(sb);

	if (!sxml)
	{
		fprintf(stderr, "%s[%d]:  FIXME:  called xml function with non xml serializer\n",
				FILE__, __LINE__);
		return false;
	}

	SerDesXML sdxml = sxml->getSD_xml();
	start_xml_elem(sdxml, tag);
	return true;

}

SerDesXML &SerializerXML::getSD_xml()
{
	SerializerBase *sb = this;
	SerDes &sd = sb->getSD();
	SerDesXML *sdxml = dynamic_cast<SerDesXML *> (&sd);
	assert(sdxml);
	return *sdxml;
}

bool SerializerXML::end_xml_element(SerializerBase * sb, const char  * /*tag*/)
{
   SerializerXML *sxml = dynamic_cast<SerializerXML *>(sb);

   if (!sxml) 
   {
      fprintf(stderr, "%s[%d]:  FIXME:  called xml function with non xml serializer\n", 
            FILE__, __LINE__);
      return false;
   }

   SerDesXML sdxml = sxml->getSD_xml();
   end_xml_elem(sdxml);

#if 0
   sdxml.end_element(); 
#endif
   return true;
}

SerDesXML::~SerDesXML()
{

}

void SerDesXML::vector_start(unsigned long &/*size*/, const char *tag) DECLTHROW(SerializerError)
{
   bool rc = ::start_xml_elem(writer, tag);

   if (!rc)
   {
        SER_ERR("testXmlwriterDoc: Error at my_xmlTextWriterStartElement");
   }

}

void SerDesXML::vector_end()
{
   bool rc = ::end_xml_elem(writer);
   if (!rc) 
   {
      SER_ERR("testXmlwriterDoc: Error at my_xmlTextWriterStartElement");
   }

}

void SerDesXML::multimap_start(unsigned long &/*size*/, const char *tag) DECLTHROW(SerializerError)
{
   bool rc = ::start_xml_elem(writer, tag);

   if (!rc)
   {
        SER_ERR("testXmlwriterDoc: Error at my_xmlTextWriterStartElement");
   }
}

void SerDesXML::multimap_end()
{
   bool rc = ::end_xml_elem(writer);
   if (!rc) 
   {
      SER_ERR("testXmlwriterDoc: Error at my_xmlTextWriterStartElement");
   }

}

void SerDesXML::pair_start(const char *tag) DECLTHROW(SerializerError)
{
   bool rc = ::start_xml_elem(writer, tag);

   if (!rc)
   {
        SER_ERR("testXmlwriterDoc: Error at my_xmlTextWriterStartElement");
   }
}

void SerDesXML::pair_end()
{
   bool rc = ::end_xml_elem(writer);
   if (!rc) 
   {
      SER_ERR("testXmlwriterDoc: Error at my_xmlTextWriterStartElement");
   }

}
void SerDesXML::hash_map_start(unsigned long &/*size*/, const char *tag) DECLTHROW(SerializerError)
{
   bool rc = ::start_xml_elem(writer,  tag);

   if (!rc)
   {
        SER_ERR("testXmlwriterDoc: Error at my_xmlTextWriterStartElement");
   }
}

void SerDesXML::hash_map_end()
{
   bool rc = ::end_xml_elem(writer);
   if (!rc) 
   {
      SER_ERR("testXmlwriterDoc: Error at my_xmlTextWriterStartElement");
   }

}

void SerDesXML::annotation_start(Dyninst::AnnotationClassID &a_id, void *& parent_id, sparse_or_dense_anno_t &sod, const char * /*id*/, const char * tag) 
{
   bool rc = ::start_xml_elem(writer, tag);

   if (!rc)
   {
        SER_ERR("testXmlwriterDoc: Error at my_xmlTextWriterStartElement");
   }
   translate(a_id, "annotationID");
   translate((Address &)parent_id, "annotatableID");
   translate((int &) sod, "SparseOrDense");
   //char sodstr[12];
   //sprintf(sodstr, "%s", sod == sparse ? "sparse" : "dense");
   //const char *sodstr = (sod == sparse) ? "sparse" : "dense";
   //translate((const char *&)const_cast<const char *>(sodstr), 12, "SparseOrDense");
}

void SerDesXML::annotation_end()
{
   bool rc = ::end_xml_elem(writer);
   if (!rc) 
   {
      SER_ERR("testXmlwriterDoc: Error at my_xmlTextWriterStartElement");
   }

}

void SerDesXML::annotation_container_start(void *& id) 
{
   bool rc = ::start_xml_elem(writer, "AnnotationContainer");

   if (!rc)
   {
        SER_ERR("testXmlwriterDoc: Error at my_xmlTextWriterStartElement");
   }
   translate((Address &)id, "containerID");
}

void SerDesXML::annotation_container_end()
{
   bool rc = ::end_xml_elem(writer);
   if (!rc) 
   {
      SER_ERR("testXmlwriterDoc: Error at my_xmlTextWriterStartElement");
   }

}

void SerDesXML::annotation_container_item_start(void *& id) 
{
   bool rc = ::start_xml_elem(writer, "AnnotationContainerItem");

   if (!rc)
   {
        SER_ERR("testXmlwriterDoc: Error at my_xmlTextWriterStartElement");
   }
   translate((Address &)id, "containerID");
}

void SerDesXML::annotation_container_item_end()
{
   bool rc = ::end_xml_elem(writer);
   if (!rc) 
   {
      SER_ERR("testXmlwriterDoc: Error at my_xmlTextWriterStartElement");
   }

}
void SerDesXML::annotation_list_start(Address &/*id*/, unsigned long &/*nelem*/, const char * tag) 
{
   bool rc = ::start_xml_elem(writer, tag);

   if (!rc)
   {
        SER_ERR("testXmlwriterDoc: Error at my_xmlTextWriterStartElement");
   }
}

void SerDesXML::annotation_list_end()
{
   bool rc = ::end_xml_elem(writer);
   if (!rc) 
   {
      SER_ERR("testXmlwriterDoc: Error at my_xmlTextWriterStartElement");
   }

}
void SerDesXML::translate(bool &param, const char *tag)
{       
   bool rc = write_xml_elem(writer, tag,
         "%s", param ? "true" : "false");

   if (!rc) 
   {
      SER_ERR("testXmlwriterDoc: Error at my_xmlTextWriterStartElement");
   }

}     

void SerDesXML::translate(char &param, const char *tag)
{       
   bool rc = write_xml_elem(writer, tag,
         "%c", param);

   if (!rc) 
   {
      SER_ERR("testXmlwriterDoc: Error at my_xmlTextWriterStartElement");
   }

}     

void SerDesXML::translate(int &param, const char *tag)
{   
   bool rc = write_xml_elem(writer, tag,
         "%d", param);

   if (!rc) 
   {
      SER_ERR("testXmlwriterDoc: Error at my_xmlTextWriterStartElement");
   }
    
}

void SerDesXML::translate(long &param, const char *tag)
{   
   bool rc = write_xml_elem(writer, tag,
         "%l", param);

   if (!rc) 
   {
      SER_ERR("testXmlwriterDoc: Error at my_xmlTextWriterStartElement");
   }
    
}

void SerDesXML::translate(short &param, const char *tag)
{   
   bool rc = write_xml_elem(writer, tag,
         "%h", param);

   if (!rc) 
   {
      SER_ERR("testXmlwriterDoc: Error at my_xmlTextWriterStartElement");
   }
    
}

void SerDesXML::translate(unsigned short &param, const char *tag)
{   
   bool rc = write_xml_elem(writer, tag,
         "%h", param);

   if (!rc) 
   {
      SER_ERR("testXmlwriterDoc: Error at my_xmlTextWriterStartElement");
   }
    
}
void SerDesXML::translate(unsigned int &param, const char *tag)
{   
  translate( param, tag);
}

void SerDesXML::translate(float &param, const char *tag)
{
   bool rc = write_xml_elem(writer, tag,
         "%e", param);

   if (!rc) 
   {
      SER_ERR("testXmlwriterDoc: Error at my_xmlTextWriterStartElement");
   }

}

void SerDesXML::translate(double &param, const char *tag)
{
   bool rc = write_xml_elem(writer, tag,
         "%g", param);

   if (!rc) 
   {
      SER_ERR("testXmlwriterDoc: Error at my_xmlTextWriterStartElement");
   }

}

void SerDesXML::translate(Address &param, const char *tag)
{
   bool rc = write_xml_elem(writer, tag,
         "%p", param);

   if (!rc) 
   {
      SER_ERR("testXmlwriterDoc: Error at my_xmlTextWriterStartElement");
   }

}

void SerDesXML::translate(void * &param, const char *tag)
{
   bool rc = write_xml_elem(writer, tag,
         "%p", param);

   if (!rc) 
   {
      SER_ERR("testXmlwriterDoc: Error at my_xmlTextWriterStartElement");
   }

}
void SerDesXML::translate(const char * &param, int /*bufsize*/, const char *tag)
{
   bool rc = write_xml_elem(writer, tag,
         "%s", param);

   if (!rc) 
   {
      SER_ERR("testXmlwriterDoc: Error at my_xmlTextWriterStartElement");
   }
}

void SerDesXML::translate(char * &param, int /*bufsize*/, const char *tag)
{
   bool rc = write_xml_elem(writer, tag,
         "%s", param);

   if (!rc) 
   {
      SER_ERR("testXmlwriterDoc: Error at my_xmlTextWriterStartElement");
   }

}

void SerDesXML::translate(std::string &param, const char *tag)
{
   assert(tag);
   assert(param.c_str());

   bool rc = write_xml_elem(writer, tag,
         "%s", param.c_str());

   if (!rc) 
   {
      SER_ERR("testXmlwriterDoc: Error at my_xmlTextWriterStartElement");
   }

}

void SerDesXML::translate(std::vector<std::string> &param, const char *tag,
                          const char *elem_tag)
{
   bool rc = ::start_xml_elem(writer, tag);
   if (!rc)
   {
      SER_ERR("testXmlwriterDoc: Error at my_xmlTextWriterStartElement");
   }

    for (unsigned int i = 0; i < param.size(); ++i) 
      translate(param[i], elem_tag);
    

   rc = ::end_xml_elem(writer);
   if (!rc)
   {
      SER_ERR("testXmlwriterDoc: Error at my_xmlTextWriterEndElement");
   }

}

#endif
