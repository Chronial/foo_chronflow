#pragma once

//#define COVER_CONFIG_DEF_CONTENT "Blubalutsch\r\n" \
//			"Hier muss noch eine sinnvolle Beschreibung rein"

#include "COVER_CONFIG_DEF_CONTENT.h"

struct CoverConfig {
	pfc::string8 name;
	pfc::string8 script;
};

class cfg_coverConfigs :
	public cfg_var, public pfc::list_t<CoverConfig>
{
public:
	cfg_coverConfigs(const GUID& p_guid, char const* const* p_val) : cfg_var(p_guid)
	{
		int i = 0;
		while (**p_val != '\0'){
			m_buffer.set_size(i + 1);
			m_buffer[i].name.set_string(*p_val);
			p_val++;
			m_buffer[i].script.set_string(*p_val);
			p_val++;
			i++;
		}
	}
	CoverConfig* getPtrByName(const char* name){
		int count = get_count();
		for (int i=0; i < count; i++){
			if (!stricmp_utf8(m_buffer[i].name, name)){
				return &m_buffer[i];
			}
		}
		return false;
	}
	bool removeItemByName(const char* name){
		int count = get_count();
		for (int i=0; i < count; i++){
			if (!stricmp_utf8(m_buffer[i].name, name)){
				remove_by_idx(i);
				return true;
			}
		}
		return false;
	}
protected:
	void get_data_raw(stream_writer * p_stream, abort_callback & p_abort)
	{
		int c = get_count();
		p_stream->write_lendian_t(version, p_abort);
		p_stream->write_lendian_t(c, p_abort);
		for (int i=0; i < c; i++){
			const CoverConfig config = get_item_ref(i);
			p_stream->write_string(config.name, p_abort);
			p_stream->write_string(config.script, p_abort);
		}
	}

	void set_data_raw(stream_reader * p_stream,t_size p_sizehint,abort_callback & p_abort)
	{
		int c, v;
		p_stream->read_lendian_t(v, p_abort);
		assert(v == version);
		p_stream->read_lendian_t(c, p_abort);
		m_buffer.set_size(c);
		for (int i=0; i < c; i++){
			p_stream->read_string(m_buffer[i].name, p_abort);
			p_stream->read_string(m_buffer[i].script, p_abort);
		}
	}
private:
	static const unsigned int version = 1;
};

/*
//! Struct config variable template. Warning: not endian safe, should be used only for nonportable code.\n
template<typename t_struct>
class cfg_struct_ptr_t : public cfg_var {
private:
	t_struct* m_val;
protected:

	void get_data_raw(stream_writer * p_stream,abort_callback & p_abort) {
		t_struct* temp = m_val;
		if (temp == 0){
			temp = new t_struct();
			memset(temp, 0, sizeof(t_struct));
		}
		p_stream->write_object(m_val,sizeof(t_struct),p_abort);
	}
	void set_data_raw(stream_reader * p_stream,t_size p_sizehint,abort_callback & p_abort) {
		t_struct* temp = new t_struct();
		p_stream->read_object(temp,sizeof(t_struct),p_abort);
		m_val = temp;
	}
public:
	inline cfg_struct_ptr_t(const GUID & p_guid, t_struct* p_val)
		: cfg_var(p_guid), m_val(p_val) {}
	
	inline const cfg_struct_ptr_t<t_struct> & operator=(t_struct* p_val) {
		m_val = p_val;
		return *this;
	}

	inline bool isEmpty() const {
		return m_val == 0;
	}

	//inline const t_struct* get_value() const {return m_val;}
	//inline t_struct& get_value() {return m_val;}
	inline operator t_struct*() const {return m_val;}
};*/