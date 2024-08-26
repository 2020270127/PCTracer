#pragma once
#include "typedef.h"
#include "strconv.h"

namespace peparser
{
	using namespace strconv;

	enum PE_DIRECTORY_TYPE
	{
		PE_DIRECTORY_ALL = 0xff,
		PE_DIRECTORY_EAT = 0x1,
		PE_DIRECTORY_IAT = 0x2,
		PE_DIRECTORY_TLS = 0x4,
		PE_DIRECTORY_DEBUG = 0x8
	};

	typedef struct _SECTION_INFO
	{
		tstring Name;
		size_t VirtualAddress;
		size_t PointerToRawData;
		size_t SizeOfRawData;
		size_t Characteristics;
		size_t RealAddress;
	}
	SECTION_INFO, * PSECTION_INFO;
	typedef vector<SECTION_INFO> SectionList;

	typedef struct _FUNCTION_INFO
	{
		tstring Name;
		size_t Ordinal;
		size_t Address;
	}
	FUNCTION_INFO, * PFUNCTION_INFO;
	typedef vector<FUNCTION_INFO> Functionist;

	typedef vector<tuple<tstring, Functionist>> ModuleFunctionist;

#define IMAGE_PDB_SIGNATURE 0x53445352 // "RSDS"
	typedef struct _IMAGE_PDB_INFO
	{
		DWORD Signature;
		BYTE Guid[16];
		DWORD Age;
		BYTE PdbFileName[1];
	}
	IMAGE_PDB_INFO, * PIMAGE_PDB_INFO;

	typedef struct _PDB_FILE_INFO
	{
		tstring FilePath;
		u8string u8FilePath;
	}
	PDB_FILE_INFO, * PPDB_FILE_INFO;

	typedef struct _TLS_CALLBACK
	{
		size_t TlsCallbackAddress;
	}
	TLS_CALLBACK, * PTLS_CALLBACK;

	typedef vector<TLS_CALLBACK> TlsCallbackList;

	typedef struct _PE_STRUCT
	{
		bool is32Bit;
		size_t baseAddress;
		size_t imageBase;
		size_t sizeOfHeaders;
		size_t numberofSections;
		PIMAGE_DOS_HEADER dosHeader;
		PIMAGE_FILE_HEADER fileHader;
		union
		{
			PIMAGE_NT_HEADERS32 ntHeader32;
			PIMAGE_NT_HEADERS64 ntHeader64;
		};
		PIMAGE_SECTION_HEADER sectionHeader;
		PIMAGE_DATA_DIRECTORY dataDirectory;
		SectionList sectionList;
		ModuleFunctionist exportFunctionList;
		ModuleFunctionist importFunctionList;
		PDB_FILE_INFO pdbFileInfo;
		TlsCallbackList tlsCallbackList;
		tstring filePath;
	}
	PE_STRUCT;

	class IPEBase
	{
	public:
		virtual ~IPEBase() = default;
		virtual bool isPE(void) const = 0;
		virtual bool is32bit(void) const = 0;
		virtual const tstring getFilePath(void) const = 0;
		virtual size_t getBaseAddress(void) const = 0;
		virtual bool getRealAddress(const size_t& rva, size_t& raw) const = 0;
		virtual bool getData(const size_t& address, const size_t& size, BinaryData& data) const = 0;
		virtual void setHeaderSize(const size_t& sizeOfHeaders) = 0;
		virtual void setSectionList(const SectionList& sectionLists) = 0;
	};
};
