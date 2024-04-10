#include <sys/stat.h>
#include <cstdlib>

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

struct str_buffer
{
	char* Data;
	u64 Size;
};

str_buffer ReadTextFile(const char* FileName)
{
	str_buffer Result = {};
    FILE* File = fopen(FileName, "rt");
    if(File)
    {
		#if _WIN32
        struct __stat64 Stat;
        _stat64(FileName, &Stat);
		#else
        struct stat Stat;
        stat(FileName, &Stat);
		#endif
        
        Result.Data = (char*)malloc(sizeof(char) * Stat.st_size + 1);
		Result.Size = Stat.st_size;
        if(Result.Data)
        {
			size_t NumChars = fread(Result.Data, sizeof(char), Stat.st_size, File);
			if (ferror(File))
			{
				fprintf(stderr, "ERROR: Unable to read '%s' into string buffer.\n", FileName);
				free(Result.Data);
				Result.Data = nullptr;
			}
			else
			{
				Assert(NumChars <= sizeof(char) * Stat.st_size);
				Result.Data[NumChars] = 0;
			}
        }
        
        fclose(File);
    }
    else
    {
        fprintf(stderr, "ERROR: Unable to open '%s' for reading.\n", FileName);
    }
    
    return Result;
}

bool WriteJsonToFile(rapidjson::Document* JsonDoc, char* FilePath, u32 SizeHint = 0)
{
	rapidjson::StringBuffer OutStringBuffer(0, SizeHint);
	rapidjson::Writer<rapidjson::StringBuffer> JsonWriter(OutStringBuffer);
	if (!JsonDoc->Accept(JsonWriter))
	{
		fprintf(stderr, "ERROR: Failed to convert JSON document to string for file '%s'.\n", FilePath);
		return false;
	}
	
	FILE* OutFile = fopen(FilePath, "wb");
	if (!OutFile)
	{
		fprintf(stderr, "ERROR: Failed to open file '%s' for writing.\n", FilePath);
		return false;
	}

	if (fprintf(OutFile, "%s", OutStringBuffer.GetString()) < 0)
	{
		fprintf(stderr, "ERROR: Failed to write to file '%s'.\n", FilePath);
		return false;
	}
	fclose(OutFile);

	return true;
}

#if _WIN32
#include <Windows.h>
bool ChangeWorkingDir(char* DirPath)
{
	if (!SetCurrentDirectory(DirPath))
	{
		fprintf(stderr, "ERROR: Failed to change working directory to '%s'\n", DirPath);
		return false;
	}
	return true;
}
#else
#include <unistd.h>
bool ChangeWorkingDir(char* DirPath)
{
	if (chdir(DirPath) != 0)
	{
		fprintf(stderr, "ERROR: Failed to change working directory to '%s'\n", DirPath);
		return false;
	}
	return true;
}
#endif

void GetFullPath(const char* RelPath, char* OutFullPath)
{
	char* Result;
#if _WIN32
	Result = _fullpath(OutFullPath, RelPath, MAX_PATH);
#else
	Result = realpath(RelPath, OutFullPath);
#endif
	if (!Result)
	{
		fprintf(stderr, "Failed to get absolute path for '%s'\n.", RelPath);
	}
}

void AppendToFilePath(const char* FilePath, char* Suffix, char* OutPath)
{
	u32 IndexOfLastDot = 0;
	for (u32 i = 0; FilePath[i]; i++)
	{
		if (FilePath[i] == '.')
		{
			IndexOfLastDot = i;
		}
	}

	u32 OutIndex = 0;
	for (u32 i = 0; i < IndexOfLastDot; i++)
	{
		OutPath[OutIndex++] = FilePath[i];
	}
	for (u32 i = 0; Suffix[i]; i++)
	{
		OutPath[OutIndex++] = Suffix[i];
	}
	for (u32 i = IndexOfLastDot; FilePath[i]; i++)
	{
		OutPath[OutIndex++] = FilePath[i];
	}
	OutPath[OutIndex] = 0;
}

void StripFileName(const char* FilePath, char* OutStr)
{
	u32 IndexOfLastSlash = 0;
	for (u32 i = 0; FilePath[i]; i++)
	{
		if (FilePath[i] == '/' || FilePath[i] == '\\')
		{
			IndexOfLastSlash = i;
		}
	}

	u32 i;
	for (i = 0; i < IndexOfLastSlash; i++)
	{
		OutStr[i] = FilePath[i];
	}
	OutStr[i] = 0;
}

void GetFileExtension(const char* FilePath, char* OutExt)
{
	u32 IndexOfLastDot = 0;
	for (u32 i = 0; FilePath[i]; i++)
	{
		if (FilePath[i] == '.')
		{
			IndexOfLastDot = i;
		}
	}
	u32 OutIndex = 0;
	for (u32 i = IndexOfLastDot; FilePath[i]; i++)
	{
		OutExt[OutIndex++] = FilePath[i];
	}
	OutExt[OutIndex] = 0;
}

void StripFileExtension(const char* FilePath, char* OutStr)
{
	u32 IndexOfLastDot = 0;
	for (u32 i = 0; FilePath[i]; i++)
	{
		if (FilePath[i] == '.')
		{
			IndexOfLastDot = i;
		}
	}
	u32 i;
	for (i = 0; i < IndexOfLastDot; i++)
	{
		OutStr[i] = FilePath[i];
	}
	OutStr[i] = 0;
}

void ExtractBaseFileName(const char* FilePath, char* OutName)
{
	s32 IndexOfLastSlash = -1;
	for (s32 i = 0; FilePath[i]; i++)
	{
		if (FilePath[i] == '/' || FilePath[i] == '\\')
		{
			IndexOfLastSlash = i;
		}
	}

	s32 OutIndex = 0;
	for (u32 i = IndexOfLastSlash + 1; FilePath[i]; i++)
	{
		OutName[OutIndex++] = FilePath[i];
	}
	OutName[OutIndex] = 0;
}