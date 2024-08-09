#include "peparser.h"

namespace peparser
{
	using namespace std;

	PEParser::PEParser() :
		peBase_(nullptr), peStruct_({ 0, })
	{
		logger_.setLogType(LOG_LEVEL_ERROR, LOG_DIRECTION_CONSOLE, TRUE);
	};

	PEParser::~PEParser()
	{
		close();
	};

	void PEParser::close(void)
	{
		// 생성된 객체(PEFile or PEProcess) 해제
		delete peBase_;
		peBase_ = nullptr;

		// peStruct_ 멤버 초기화
		peStruct_.baseAddress = 0;
		peStruct_.imageBase = 0;
		peStruct_.sizeOfHeaders = 0;
		peStruct_.numberofSections = 0;
		peStruct_.dosHeader = nullptr;
		peStruct_.fileHader = nullptr;
		peStruct_.ntHeader32 = nullptr;
		peStruct_.sectionHeader = nullptr;
		peStruct_.dataDirectory = nullptr;
		peStruct_.sectionList.clear();
		peStruct_.exportFunctionList.clear();
		peStruct_.importFunctionList.clear();
	};

	bool PEParser::open(const tstring_view& filePath)
	{
		PEFile* peFile = new PEFile();
		peBase_ = reinterpret_cast<IPEBase*>(peFile);
		return peFile->open(filePath);
	};

	bool PEParser::parseEATCustom(const PE_DIRECTORY_TYPE& parseType)
	{
		if (peBase_->isPE())
		{
			// Set base address
			peStruct_.baseAddress = peBase_->getBaseAddress();

			// PE header parsing
			parseHeaders();

			// PE DataDirectory entry parsing
			if ((parseType & PE_DIRECTORY_EAT) == PE_DIRECTORY_EAT)
			{
				parseEAT();
			};

			return true;
		}
		else
		{
			return false;
		}
	};


	// PE 정보를 담은 구조체 리턴
	const PE_STRUCT& PEParser::getPEStructure(void) const
	{
		return peStruct_;
	};

	// PE 파일의 문자열은 UTF8 문자열이기 때문에 변환 필요
	tstring PEParser::getPEString(const char8_t* peStr)
	{
		return strConv_.to_tstring(peStr);
	};

	// 섹션 이름 같은 경우 최대 8바이트를 이용해서 이름이 저장되어 있는데
	// NULL 문자('\0') 없이 전체 바이트 모두 문자로 차 있을 수 있기 때문에 
	// 이 부분을 감안해서 처리 필요
	tstring PEParser::getPEString(const char8_t* peStr, const size_t& maxLength)
	{
		// UTF8 문자열을 생성(여기에는 NULL 문자가 여러 개 포함된 상태일 수 있음)
		const u8string u8string(peStr, maxLength);

		// c_str()을 통해서 불필요한 NULL 문자를 제거한 상태로 문자열 변환
		// ( c_str()은 NULL 문자가 포함된 문자열을 리턴 )
		return strConv_.to_tstring(u8string.c_str());
	};

	void PEParser::parseSectionHeaders(void)
	{
		size_t realAddress = 0;
		PIMAGE_SECTION_HEADER sectionHeader = peStruct_.sectionHeader;

		for (WORD index = 0; index < peStruct_.numberofSections; index++)
		{
			// 섹션의 실제 주소를 구함

			if (typeid(*peBase_) == typeid(PEFile))
			{
				// 파일의 섹션의 경우 RVA가 VirtualAddress이기 때문에 PointerToRawData가 RAW
				// RAW = RVA - VirtualAddress + PointerToRawData
				realAddress = peBase_->getBaseAddress() + sectionHeader->PointerToRawData;
			}

			// 섹션의 주요 정보를 담은 SECTION_INFO 구조체를 sectionList_에 추가
			peStruct_.sectionList.push_back(
				SECTION_INFO{
					getPEString(reinterpret_cast<const char8_t*>(sectionHeader->Name),
					sizeof(sectionHeader->Name)),
					static_cast<size_t>(sectionHeader->VirtualAddress),
					static_cast<size_t>(sectionHeader->PointerToRawData),
					static_cast<size_t>(sectionHeader->SizeOfRawData),
					static_cast<size_t>(sectionHeader->Characteristics),
					realAddress
				}
			);

			// 다음 섹션 헤더로 포인터 이동
			sectionHeader++;
		}

		// RVA to RAW 계산 등을 위해서 필요한 섹션 정보를 설정
		peBase_->setSectionList(peStruct_.sectionList);
	};

	void PEParser::parseHeaders(void)
	{
		peStruct_.dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(peStruct_.baseAddress);
		peStruct_.is32Bit = peBase_->is32bit();

		if (peStruct_.is32Bit)
		{
			peStruct_.ntHeader32 = reinterpret_cast<PIMAGE_NT_HEADERS32>(peStruct_.baseAddress + peStruct_.dosHeader->e_lfanew);
			peStruct_.fileHader = reinterpret_cast<PIMAGE_FILE_HEADER>(&(peStruct_.ntHeader32->FileHeader));
			peStruct_.sectionHeader = reinterpret_cast<PIMAGE_SECTION_HEADER>(peStruct_.baseAddress + peStruct_.dosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS32));
			peStruct_.imageBase = peStruct_.ntHeader32->OptionalHeader.ImageBase;
			peStruct_.dataDirectory = peStruct_.ntHeader32->OptionalHeader.DataDirectory;
			peStruct_.sizeOfHeaders = peStruct_.ntHeader32->OptionalHeader.SizeOfHeaders;
		}
		else
		{
			peStruct_.ntHeader64 = reinterpret_cast<PIMAGE_NT_HEADERS64>(peStruct_.baseAddress + peStruct_.dosHeader->e_lfanew);
			peStruct_.fileHader = reinterpret_cast<PIMAGE_FILE_HEADER>(&(peStruct_.ntHeader64->FileHeader));
			peStruct_.sectionHeader = reinterpret_cast<PIMAGE_SECTION_HEADER>(peStruct_.baseAddress + peStruct_.dosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS64));
			peStruct_.imageBase = peStruct_.ntHeader64->OptionalHeader.ImageBase;
			peStruct_.dataDirectory = peStruct_.ntHeader64->OptionalHeader.DataDirectory;
			peStruct_.sizeOfHeaders = peStruct_.ntHeader64->OptionalHeader.SizeOfHeaders;
		}
		peStruct_.numberofSections = peStruct_.fileHader->NumberOfSections;

		// RvaToRaw 계산 등을 위해서 헤더들의 전체 크기(PE Header + Section Header) 설정
		peBase_->setHeaderSize(peStruct_.sizeOfHeaders);

		// Parse section header
		parseSectionHeaders();
	};

	void PEParser::parseEAT(void)
	{
		tstring moduleName;
		size_t exportDescriptorAddress = 0;
		size_t exportDescriptorRva = 0;
		size_t odinalBase = 0;
		size_t functionsAddress = 0;
		size_t nameOrdinalsAddress = 0;
		size_t namesAddress = 0;
		size_t realNamesAddress = 0;
		PIMAGE_EXPORT_DIRECTORY exportDirectory = nullptr;
		Functionist functionList;

		// EAT는 DataDirectory의 첫 번째
		exportDescriptorRva = peStruct_.dataDirectory[0].VirtualAddress;
		if (exportDescriptorRva != 0x0)
		{
			// PE 파일에서 Export Directory(IMAGE_EXPORT_DIRECTORY)는 하나만 존재
			if (peBase_->getRealAddress(exportDescriptorRva, exportDescriptorAddress))
			{
				exportDirectory = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(exportDescriptorAddress);

				// Export Module Name : 함수를 제공하는 exe나 dll의 이름
				if (peBase_->getRealAddress(exportDirectory->Name, realNamesAddress))
				{
					moduleName = getPEString(reinterpret_cast<const char8_t*>(realNamesAddress));
				}
				else
				{
					moduleName.clear();
					logger_.log(format(_T("Can't get module name address : RVA = 0x{:x}"), static_cast<size_t>(exportDirectory->Name)), LOG_LEVEL_ERROR);
				}

				// Ordinal 은 function address index + Base(odinalBase) 값
				odinalBase = exportDirectory->Base;

				// AddressOfFunctions : Export하는 함수의 주소들을 담고 있는 배열의 시작 주소
				// AddressOfNames : Export하는 함수 이름의 주소들을 담고 있는 배열의 시작 주소
				// AddressOfNameOrdinals : 함수 이름으로 Export하는 함수들의 함수 주소를 얻기 위해 필요한 
				//                         함수 주소 배열에서의 위치(index)들을 담고 있는 배열의 시작 주소
				if ((peBase_->getRealAddress(exportDirectory->AddressOfFunctions, functionsAddress)) &&
					(peBase_->getRealAddress(exportDirectory->AddressOfNames, namesAddress)) &&
					(peBase_->getRealAddress(exportDirectory->AddressOfNameOrdinals, nameOrdinalsAddress)))
				{
					// 이름이 존재하는 함수들 체크를 위한 벡터 functionNameList 생성
					vector<tstring> functionNameList(exportDirectory->NumberOfFunctions);

					// 이름이 존재하는 함수들의 이름을 functionNameList에 저장
					// 이름이 존재하지 않는 함수들은 공백으로 저장
					for (DWORD index = 0; index < exportDirectory->NumberOfNames; index++)
					{
						if (peBase_->getRealAddress(reinterpret_cast<DWORD*>(namesAddress)[index], realNamesAddress))
						{
							functionNameList[reinterpret_cast<WORD*>(nameOrdinalsAddress)[index]] 
								= getPEString(reinterpret_cast<char8_t*>(realNamesAddress));
						}
					}

					// 전체 함수들의 정보를 목록에 저장
					for (DWORD index = 0; index < exportDirectory->NumberOfFunctions; index++)
					{
						if (functionNameList[index].empty())
						{
							// Ordinal로만 Export하는 함수 추가
							if (reinterpret_cast<DWORD*>(functionsAddress)[index] != 0)
							{
								functionList.push_back(
									FUNCTION_INFO{
										tstring(_T("")),
										static_cast<size_t>(odinalBase + index),
										static_cast<size_t>(reinterpret_cast<DWORD*>(functionsAddress)[index])
									});
							}
							else
							{
								logger_.log(format(_T("Export address is invalid > 0x{:x}, 0x{:x}"), 
									reinterpret_cast<DWORD*>(functionsAddress)[index], (WORD)odinalBase + index), LOG_LEVEL_ERROR);
							}
						}
						else
						{
							// 이름이 존재하는 함수 추가
							functionList.push_back(
								FUNCTION_INFO{
									functionNameList[index],
									static_cast<size_t>(odinalBase + index),
									static_cast<size_t>(reinterpret_cast<DWORD*>(functionsAddress)[index])
								});
						}
					}
				}
				peStruct_.exportFunctionList.push_back(make_tuple(moduleName, functionList));
			}
		}
	};

};
