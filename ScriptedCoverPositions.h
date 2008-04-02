#pragma once
struct glTextureCoords {
	float u;
	float v;
};
union glVectord;
struct glVertex { //GL_V3F
	float x;
	float y;
	float z;
	operator glVectord ();
	glVertex operator+ (const glVertex& v) const{
		glVertex res;
		res.x = x + v.x;
		res.y = y + v.y;
		res.z = z + v.z;
		return res;
	}
};
typedef glVertex glVectorf;
union glVectord { //GL_V3F
	struct {
		double x;
		double y;
		double z;
	};
	double contents[3];
	
	glVectord(double x, double y, double z): x(x), y(y), z(z) {};
	glVectord(){};

	glVectord operator+ (const glVectord& v) const{
		glVectord res;
		res.x = x + v.x;
		res.y = y + v.y;
		res.z = z + v.z;
		return res;
	}
	glVectord operator/ (double s) const{
		glVectord res;
		res.x = x/s;
		res.y = y/s;
		res.z = z/s;
		return res;
	}
	glVectord operator* (double s) const{
		glVectord res;
		res.x = x*s;
		res.y = y*s;
		res.z = z*s;
		return res;
	}
	glVectord operator- (const glVectord& v) const{
		glVectord res;
		res.x = x - v.x;
		res.y = y - v.y;
		res.z = z - v.z;
		return res;
	}
	double operator* (const glVectord& a) const{
		return x * a.x + y * a.y + z * a.z;
	}
	glVectord cross(const glVectord& v) const{
		glVectord res;
		res.x = y * v.z - z * v.y;
		res.y = z * v.x - x * v.z;
		res.z = x * v.y - y * v.x;
		return res;
	}
	double intersectAng(const glVectord& v) const{
		return acos(((*this) * v)/(this->length() * v.length()));
	}
	double length() const{
		return sqrt(x*x + y*y + z*z);
	}
	glVectord normalize() const{
		double l = length();
		if (l > 0)
			return (*this) / l;
		else
			return (*this);
	}
	operator glVectorf (){
		glVectorf res;
		res.x = (float)x;
		res.y = (float)y;
		res.z = (float)z;
		return res;
	}
};

inline glVectord operator* (double s, const glVectord& v){
	return v * s;
}

inline glVertex::operator glVectord (){
	glVectord res;
	res.x = (double)x;
	res.y = (double)y;
	res.z = (double)z;
	return res;
}


struct glTexturedVertex { //GL_T2F_V3F
	glTextureCoords tCoords;
	glVertex vCoords;
};
struct glTexturedQuad { //GL_T2F_V3F
	glTexturedVertex topLeft;
	glTexturedVertex topRight;
	glTexturedVertex bottomRight;
	glTexturedVertex bottomLeft;
};


union glMatrix_3x3 {
	//glVectord columns[3];
	double contents[9];
	double matrix[3][3];
	static glMatrix_3x3 getRotationMatrix(double a, glVectord axis){
		glMatrix_3x3 out;
		double cosa = cos(a);
		double sina = sin(a);
		out.contents[0] = cosa + axis.x * axis.x * (1 - cosa);
		out.contents[1] = axis.y * axis.x * (1 - cosa) + axis.z * sina;
		out.contents[2] = axis.z * axis.x * (1 - cosa) - axis.y * sina;

		out.contents[3] = axis.x * axis.y * (1 - cosa) - axis.z * sina;
		out.contents[4] = cosa + axis.y * axis.y * (1 - cosa);
		out.contents[5] = axis.z * axis.y * (1 - cosa) + axis.x * sina;

		out.contents[6] = axis.x * axis.z * (1 - cosa) + axis.y * sina;
		out.contents[7] = axis.y * axis.z * (1 - cosa) - axis.x * sina;
		out.contents[8] = cosa + axis.z * axis.z * (1 - cosa);
		return out;
	}
	glVectord operator*(const glVectord& v) const {
		glVectord out;
		for( int i=0; i < 3; i++ ){
			out.contents[i] = 0;
			for (int j=0; j < 3; j++){
				out.contents[i] += this->matrix[j][i] * v.contents[j];
			}
		}
		return out;
	}
};



struct glQuad { //GL_V3F
	glVertex topLeft;
	glVertex topRight;
	glVertex bottomRight;
	glVertex bottomLeft;
	void rotate(double a, glVectord axis){
		glMatrix_3x3 trans = glMatrix_3x3::getRotationMatrix(a, axis);
		topLeft = trans * topLeft;
		topRight = trans * topRight;
		bottomLeft = trans * bottomLeft;
		bottomRight = trans * bottomRight;
	}
};

struct fovAspectBehaviour {
	float x;
	float y;
};

struct CoverPosInfo { // When changing this you have to update CompiledCPInfo::version
	glVectorf position;

	struct {
		float a;
		glVectorf axis;
	} rotation;

	struct {
		float x;
		float y;
	} alignment;

	struct {
		float w;
		float h;
	} sizeLim;

	static CoverPosInfo interpolate(const CoverPosInfo& a, const CoverPosInfo& b, float bWeight){
		CoverPosInfo out;

		out.position.x = interpolF(a.position.x, b.position.x, bWeight);
		out.position.y = interpolF(a.position.y, b.position.y, bWeight);
		out.position.z = interpolF(a.position.z, b.position.z, bWeight);

		out.rotation.a = interpolF(a.rotation.a, b.rotation.a, bWeight);
		out.rotation.axis.x = interpolF(a.rotation.axis.x, b.rotation.axis.x, bWeight);
		out.rotation.axis.y = interpolF(a.rotation.axis.y, b.rotation.axis.y, bWeight);
		out.rotation.axis.z = interpolF(a.rotation.axis.z, b.rotation.axis.z, bWeight);

		out.alignment.x = interpolF(a.alignment.x, b.alignment.x, bWeight);
		out.alignment.y = interpolF(a.alignment.y, b.alignment.y, bWeight);

		out.sizeLim.w = interpolF(a.sizeLim.w, b.sizeLim.w, bWeight);
		out.sizeLim.h = interpolF(a.sizeLim.h, b.sizeLim.h, bWeight);

		return out;
	}
private:
	static inline float interpolF(float a, float b, float bWeight){
		return a*(1 - bWeight)  +  b*bWeight;
	}
};

class CompiledCPInfo {
	static const int tableRes = 20; // if you change this, you have to change version
	
	friend class CPScriptCompiler;

public:
	static const int version = 1;

	bool showMirrorPlane;
	glVectord mirrorNormal; // guaranteed to have length 1
	glVectord mirrorCenter;

	glVectord cameraPos;
	glVectord lookAt;
	glVectord upVector;

	int firstCover;
	int lastCover;

	fovAspectBehaviour aspectBehaviour;

	// this has to be the last Member!
	pfc::array_t<CoverPosInfo> coverPosInfos;

	CoverPosInfo getCoverPosInfo(float coverIdx){
		float idx = coverIdx - firstCover;
		idx *= tableRes;
		int iPart = static_cast<int>(idx);
		float fPart = idx - iPart;
		
		return CoverPosInfo::interpolate(coverPosInfos[iPart], coverPosInfos[iPart+1], fPart);
	}

	void serialize(stream_writer * p_stream, abort_callback & p_abort){
		p_stream->write_lendian_t(version, p_abort);

		p_stream->write_object((void*)this, offsetof(CompiledCPInfo, coverPosInfos), p_abort);

		t_size c = coverPosInfos.get_size();
		p_stream->write_lendian_t(c, p_abort);
		p_stream->write_object((void*)coverPosInfos.get_ptr(), sizeof(CoverPosInfo)*c, p_abort);
	}
	static void unserialize(CompiledCPInfo& out, stream_reader* p_stream, abort_callback & p_abort){
		int fileVer;
		p_stream->read_lendian_t(fileVer, p_abort);
		PFC_ASSERT(fileVer == version);
		p_stream->read_object((void*)&out, offsetof(CompiledCPInfo, coverPosInfos), p_abort);

		t_size c;
		p_stream->read_lendian_t(c, p_abort);
		CoverPosInfo* tmp = new CoverPosInfo[c];
		p_stream->read_object((void*)tmp, sizeof(CoverPosInfo) * c, p_abort);
		out.coverPosInfos.set_data_fromptr(tmp, c);
		delete tmp;
	}
};

struct CPScriptFuncInfos {
	static const t_size funcCount = 12;
	static const wchar_t* knownFunctions[funcCount];

	static const bit_array_range neededFunctions;
	static const bit_array_range mirrorFunctions;
} ;

class CPScriptCompiler {
public:
	CPScriptCompiler();
	bool compileScript(const char *, CompiledCPInfo& out, pfc::string_base& message);

private:
	CScriptObject scriptObj;
	bool scShowMirrorPlane(bool& res, pfc::string_base& message);
	bool scMirrorSize(double& res, pfc::string_base& message);
	bool scCallDArrayFunction(const wchar_t* func, pfc::list_t<double>& res, pfc::string_base& message);
	bool scCallDArrayFunction(const wchar_t* func, pfc::list_t<double>& res, pfc::string_base& message, double param);
	bool scCallDArrayFunction(const wchar_t* func, pfc::list_t<double>& res, pfc::string_base& message, LPSAFEARRAY sa);
};



class cfg_compiledCPInfoPtr : public cfg_var {
private:
	CompiledCPInfo * data;
protected:
	void get_data_raw(stream_writer * p_stream,abort_callback & p_abort) {

		if (data){
			p_stream->write_lendian_t(true, p_abort);
			data->serialize(p_stream, p_abort);
		} else {
			p_stream->write_lendian_t(false, p_abort);
		}
	}
	void set_data_raw(stream_reader * p_stream,t_size p_sizehint,abort_callback & p_abort) {
		bool configNotEmpty;
		p_stream->read_lendian_t(configNotEmpty, p_abort);
		if (configNotEmpty){
			data = new CompiledCPInfo;
			CompiledCPInfo::unserialize(*data, p_stream, p_abort);
		} else {
			data = 0;
		}
	}
public:
	inline cfg_compiledCPInfoPtr(const GUID & p_guid, CompiledCPInfo* p_val)
		: cfg_var(p_guid), data(p_val) {}
	
	inline const cfg_compiledCPInfoPtr & operator=(CompiledCPInfo* p_val) {
		data = p_val;
		return *this;
	}

	inline bool isEmpty() const {
		return data == 0;
	}

	inline operator CompiledCPInfo*() {return data;}
};

extern cfg_compiledCPInfoPtr sessionCompiledCPInfo;
extern cfg_coverConfigs cfgCoverConfigs;
extern cfg_string cfgCoverConfigSel;

class ScriptedCoverPositions
{
public:
	ScriptedCoverPositions(){
		if (!sessionCompiledCPInfo.isEmpty()){
			cInfo = sessionCompiledCPInfo;
		} else {
			const CoverConfig* config = cfgCoverConfigs.getPtrByName(cfgCoverConfigSel);
			pfc::string8 errorMsg;
			cInfo = 0;
			if (!config || !setScript(config->script, errorMsg)){
				if(!setScript(COVER_CONFIG_DEF_CONTENT, errorMsg)){
					popup_message::g_show(errorMsg, "JScript Compile Error", popup_message::icon_error);
					throw new pfc::exception(errorMsg);
				}
			}
		}
	}
	~ScriptedCoverPositions(){
		// don't delete cInfo since it is shared with sessionCompiledCPInfo
		// delete cInfo;
	}

	bool setScript(const char* script, pfc::string_base& errorMsg){
		CompiledCPInfo* compiled = new CompiledCPInfo;
		try {
			CPScriptCompiler compiler;
			if (compiler.compileScript(script, *compiled, errorMsg)){
				CompiledCPInfo* oldInfo = cInfo;
				cInfo = compiled;

				CompiledCPInfo* oldSessInfo = sessionCompiledCPInfo;
				sessionCompiledCPInfo = compiled;
				if (oldInfo == oldSessInfo){
					delete oldInfo;
				} else {
					delete oldInfo;
					delete oldSessInfo;
				}
				return true;
			} else {
				delete compiled;
				return false;
			}
		}  catch (_com_error){
			delete compiled;

			errorMsg = "Windows Script Control not installed. Download it from <http://www.microsoft.com/downloads/details.aspx?FamilyId=D7E31492-2595-49E6-8C02-1426FEC693AC>.";
			return false;
		}
	}
	inline const fovAspectBehaviour& getAspectBehaviour()
	{
		return cInfo->aspectBehaviour;
	}

	inline const glVectord& getLookAt()
	{
		return cInfo->lookAt;
	}

	inline const glVectord& getUpVector()
	{
		return cInfo->upVector;
	}

	inline bool isMirrorPlaneEnabled()
	{
		return cInfo->showMirrorPlane;
	}

	inline const glVectord& getMirrorNormal()
	{
		return cInfo->mirrorNormal;
	}
	inline const glVectord& getMirrorCenter()
	{
		return cInfo->mirrorCenter;
	}
	inline const glVectord& getCameraPos()
	{
		return cInfo->cameraPos;
	}
	inline const int getFirstCover()
	{
		return cInfo->firstCover;
	}
	inline const int getLastCover()
	{
		return cInfo->lastCover;
	}

	double distanceToMirror(glVectord point){
		return abs((cInfo->mirrorCenter - point) * cInfo->mirrorNormal);
	}

	glQuad getCoverQuad(float coverId, float coverAspect)
	{
		CoverPosInfo cPos = cInfo->getCoverPosInfo(coverId);
		
		double sizeLimAspect = cPos.sizeLim.w / cPos.sizeLim.h;

		float w;
		float h;
		if (coverAspect > sizeLimAspect){
			w = cPos.sizeLim.w;
			h = w / coverAspect;
		} else {
			h = cPos.sizeLim.h;
			w = h * coverAspect;
		}

		glQuad out;

		//out.topLeft.x = -w/2 -align.x * w/2;
		out.topLeft.x = (-1 - cPos.alignment.x) * w/2;
		out.bottomLeft.x = out.topLeft.x;
		out.topRight.x = (1 - cPos.alignment.x) * w/2;
		out.bottomRight.x = out.topRight.x;
		
		out.topLeft.y = (1 - cPos.alignment.y) * h/2;
		out.topRight.y = out.topLeft.y;
		out.bottomLeft.y = (-1 - cPos.alignment.y) * h/2;
		out.bottomRight.y = out.bottomLeft.y;

		out.topLeft.z = 0;
		out.topRight.z = 0;
		out.bottomLeft.z = 0;
		out.bottomRight.z = 0;

		out.rotate(cPos.rotation.a, cPos.rotation.axis);

		out.topLeft = cPos.position + out.topLeft;
		out.topRight = cPos.position + out.topRight;
		out.bottomLeft = cPos.position + out.bottomLeft;
		out.bottomRight = cPos.position + out.bottomRight;
		
		return out;
	}

private:
	CompiledCPInfo* cInfo;
};
