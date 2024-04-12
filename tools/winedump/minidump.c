/*
 *	MiniDump dumping utility
 *
 * 	Copyright 2005 Eric Pouech
 * 	          2024 Eric Pouech for CodeWeavers
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include <stdarg.h>

#include "winedump.h"
#include "winver.h"
#include "dbghelp.h"

static void dump_mdmp_data(const MINIDUMP_LOCATION_DESCRIPTOR* md, const char* pfx)
{
    if (md->DataSize)
        dump_data(PRD(md->Rva, md->DataSize), md->DataSize, pfx);
}

static const char *get_mdmp_str(DWORD rva)
{
    const MINIDUMP_STRING*      ms;
    if (!rva)
        return "<<rva=0>>";
    if ((ms = PRD(rva, sizeof(MINIDUMP_STRING))))
        return get_unicode_str( ms->Buffer, ms->Length / sizeof(WCHAR) );
    return "<<?>>";
}

enum FileSig get_kind_mdmp(void)
{
    const DWORD*        pdw;

    pdw = PRD(0, sizeof(DWORD));
    if (!pdw) {printf("Can't get main signature, aborting\n"); return SIG_UNKNOWN;}

    if (*pdw == 0x504D444D /* "MDMP" */) return SIG_MDMP;
    return SIG_UNKNOWN;
}

void mdmp_dump(void)
{
    const MINIDUMP_HEADER*      hdr = PRD(0, sizeof(MINIDUMP_HEADER));
    const MINIDUMP_DIRECTORY*   dir;
    const void*                 stream;
    unsigned int i, idx;
    const BYTE *ptr;

    if (!hdr)
    {
        printf("Cannot get Minidump header\n");
        return;
    }

    printf("Header\n");
    printf("  Signature: %#x (%.4s)\n", hdr->Signature, (const char*)&hdr->Signature);
    printf("  Version: %#x\n", hdr->Version);
    printf("  NumberOfStreams: %u\n", hdr->NumberOfStreams);
    printf("  StreamDirectoryRva: %#x\n", (UINT)hdr->StreamDirectoryRva);
    printf("  CheckSum: %#x (%u)\n", hdr->CheckSum, hdr->CheckSum);
    printf("  TimeDateStamp: %s\n", get_time_str(hdr->TimeDateStamp));
    printf("Flags: %s\n", get_hexint64_str(hdr->Flags));

    for (idx = 0; idx < hdr->NumberOfStreams; ++idx)
    {
        dir = PRD(hdr->StreamDirectoryRva + idx * sizeof(MINIDUMP_DIRECTORY), sizeof(*dir));
        if (!dir) break;

        stream = PRD(dir->Location.Rva, dir->Location.DataSize);

        switch (dir->StreamType)
        {
        case UnusedStream:
            printf("Stream [%u]: Unused:\n", idx);
            break;
        case ThreadListStream:
            if (globals_dump_sect("thread"))
            {
                const MINIDUMP_THREAD_LIST *mtl = stream;
                const MINIDUMP_THREAD *mt = mtl->Threads;

                printf("Stream [%u]: Threads:\n", idx);
                printf("  NumberOfThreads: %u\n", mtl->NumberOfThreads);
                for (i = 0; i < mtl->NumberOfThreads; i++, mt++)
                {
                    printf("  Thread: #%d\n", i);
                    printf("    ThreadId: %#x\n", mt->ThreadId);
                    printf("    SuspendCount: %u\n", mt->SuspendCount);
                    printf("    PriorityClass: %u\n", mt->PriorityClass);
                    printf("    Priority: %u\n", mt->Priority);
                    printf("  Teb: %s\n", get_hexint64_str(mt->Teb));
                    printf("  Stack: %s +%#x\n", get_hexint64_str(mt->Stack.StartOfMemoryRange), mt->Stack.Memory.DataSize);
                    if (globals_dump_sect("content"))
                        dump_mdmp_data(&mt->Stack.Memory, "      ");
                    printf("    ThreadContext:\n");
                    dump_mdmp_data(&mt->ThreadContext, "      ");
                }
            }
            break;
        case ModuleListStream:
        case 0xFFF0:
            if (globals_dump_sect("module"))
            {
                const MINIDUMP_MODULE_LIST *mml = stream;
                const MINIDUMP_MODULE*      mm = mml->Modules;
                const char*                 p1;
                const char*                 p2;

                printf("Stream [%u]: Modules (%s):\n", idx,
                       dir->StreamType == ModuleListStream ? "PE" : "ELF");
                printf("  NumberOfModules: %u\n", mml->NumberOfModules);
                for (i = 0; i < mml->NumberOfModules; i++, mm++)
                {
                    printf("  Module #%d:\n", i);
                    printf("    BaseOfImage: %s\n", get_hexint64_str(mm->BaseOfImage));
                    printf("    SizeOfImage: %#x (%u)\n", mm->SizeOfImage, mm->SizeOfImage);
                    printf("    CheckSum: %#x (%u)\n", mm->CheckSum, mm->CheckSum);
                    printf("    TimeDateStamp: %s\n", get_time_str(mm->TimeDateStamp));
                    printf("    ModuleName: %s\n", get_mdmp_str(mm->ModuleNameRva));
                    printf("    VersionInfo:\n");
                    printf("      dwSignature: %x\n", (UINT)mm->VersionInfo.dwSignature);
                    printf("      dwStrucVersion: %x\n", (UINT)mm->VersionInfo.dwStrucVersion);
                    printf("      dwFileVersion: %d,%d,%d,%d\n",
                           HIWORD(mm->VersionInfo.dwFileVersionMS),
                           LOWORD(mm->VersionInfo.dwFileVersionMS),
                           HIWORD(mm->VersionInfo.dwFileVersionLS),
                           LOWORD(mm->VersionInfo.dwFileVersionLS));
                    printf("      dwProductVersion %d,%d,%d,%d\n",
                           HIWORD(mm->VersionInfo.dwProductVersionMS),
                           LOWORD(mm->VersionInfo.dwProductVersionMS),
                           HIWORD(mm->VersionInfo.dwProductVersionLS),
                           LOWORD(mm->VersionInfo.dwProductVersionLS));
                    printf("      dwFileFlagsMask: %x\n", (UINT)mm->VersionInfo.dwFileFlagsMask);
                    printf("      dwFileFlags: %s%s%s%s%s%s\n",
                           mm->VersionInfo.dwFileFlags & VS_FF_DEBUG ? "Debug " : "",
                           mm->VersionInfo.dwFileFlags & VS_FF_INFOINFERRED ? "Inferred " : "",
                           mm->VersionInfo.dwFileFlags & VS_FF_PATCHED ? "Patched " : "",
                           mm->VersionInfo.dwFileFlags & VS_FF_PRERELEASE ? "PreRelease " : "",
                           mm->VersionInfo.dwFileFlags & VS_FF_PRIVATEBUILD ? "PrivateBuild " : "",
                           mm->VersionInfo.dwFileFlags & VS_FF_SPECIALBUILD ? "SpecialBuild " : "");
                    if (mm->VersionInfo.dwFileOS)
                    {
                        switch (mm->VersionInfo.dwFileOS & 0x000F)
                        {
                        case VOS__BASE:     p1 = "_base"; break;
                        case VOS__WINDOWS16:p1 = "16 bit Windows"; break;
                        case VOS__PM16:     p1 = "16 bit Presentation Manager"; break;
                        case VOS__PM32:     p1 = "32 bit Presentation Manager"; break;
                        case VOS__WINDOWS32:p1 = "32 bit Windows"; break;
                        default:            p1 = "---"; break;
                        }
                        switch (mm->VersionInfo.dwFileOS & 0xF0000)
                        {
                        case VOS_UNKNOWN:   p2 = "unknown"; break;
                        case VOS_DOS:       p2 = "DOS"; break;
                        case VOS_OS216:     p2 = "16 bit OS/2"; break;
                        case VOS_OS232:     p2 = "32 bit OS/2"; break;
                        case VOS_NT:        p2 = "Windows NT"; break;
                        default:            p2 = "---"; break;
                        }
                        printf("      dwFileOS: %s running on %s\n", p1, p2);
                    }
                    else printf("      dwFileOS: 0\n");
                    switch (mm->VersionInfo.dwFileType)
                    {
                    case VFT_UNKNOWN:       p1 = "Unknown"; break;
                    case VFT_APP:           p1 = "Application"; break;
                    case VFT_DLL:           p1 = "DLL"; break;
                    case VFT_DRV:           p1 = "Driver"; break;
                    case VFT_FONT:          p1 = "Font"; break;
                    case VFT_VXD:           p1 = "VxD"; break;
                    case VFT_STATIC_LIB:    p1 = "Static Library"; break;
                    default:                p1 = "---"; break;
                    }
                    printf("      dwFileType: %s\n", p1);
                    printf("      dwFileSubtype: %u\n", (UINT)mm->VersionInfo.dwFileSubtype);
                    printf("      dwFileDate: %x%08x\n",
                           (UINT)mm->VersionInfo.dwFileDateMS, (UINT)mm->VersionInfo.dwFileDateLS);
                    printf("    CvRecord: <%u>\n", (UINT)mm->CvRecord.DataSize);
                    if (globals_dump_sect("content"))
                        dump_mdmp_data(&mm->CvRecord, "      ");
                    printf("    MiscRecord: <%u>\n", (UINT)mm->MiscRecord.DataSize);
                    if (globals_dump_sect("content"))
                        dump_mdmp_data(&mm->MiscRecord, "      ");
                    printf("    Reserved0: %s\n", get_hexint64_str(mm->Reserved0));
                    printf("    Reserved1: %s\n", get_hexint64_str(mm->Reserved1));
                }
            }
            break;
        case MemoryListStream:
            if (globals_dump_sect("memory"))
            {
                const MINIDUMP_MEMORY_LIST *mml = stream;
                const MINIDUMP_MEMORY_DESCRIPTOR*   mmd = mml->MemoryRanges;

                printf("Stream [%u]: Memory Ranges:\n", idx);
                printf("  NumberOfMemoryRanges: %u\n", mml->NumberOfMemoryRanges);
                for (i = 0; i < mml->NumberOfMemoryRanges; i++, mmd++)
                {
                    printf("  Memory Range #%d:\n", i);
                    dump_mdmp_data(&mmd->Memory, "    ");
                    printf("    Range: %s +%#x\n", get_hexint64_str(mmd->StartOfMemoryRange), mmd->Memory.DataSize);
                    if (globals_dump_sect("content"))
                        dump_mdmp_data(&mmd->Memory, "      ");
                }
            }
            break;
        case SystemInfoStream:
            if (globals_dump_sect("info"))
            {
                const MINIDUMP_SYSTEM_INFO *msi = stream;
                const char*                 str;
                char                        tmp[128];

                printf("Stream [%u]: System Information:\n", idx);
                switch (msi->ProcessorArchitecture)
                {
                case PROCESSOR_ARCHITECTURE_UNKNOWN:
                    str = "Unknown";
                    break;
                case PROCESSOR_ARCHITECTURE_INTEL:
                    strcpy(tmp, "Intel ");
                    switch (msi->ProcessorLevel)
                    {
                    case  3: str = "80386"; break;
                    case  4: str = "80486"; break;
                    case  5: str = "Pentium"; break;
                    case  6: str = "Pentium Pro/II or AMD Athlon"; break;
                    case 15: str = "Pentium 4 or AMD Athlon64"; break;
                    default: str = "???"; break;
                    }
                    strcat(tmp, str);
                    strcat(tmp, " (");
                    if (msi->ProcessorLevel == 3 || msi->ProcessorLevel == 4)
                    {
                        if (HIBYTE(msi->ProcessorRevision) == 0xFF)
                            sprintf(tmp + strlen(tmp), "%c%d", 'A' + ((msi->ProcessorRevision>>4)&0xf)-0x0a, msi->ProcessorRevision&0xf);
                        else
                            sprintf(tmp + strlen(tmp), "%c%d", 'A' + HIBYTE(msi->ProcessorRevision), LOBYTE(msi->ProcessorRevision));
                    }
                    else sprintf(tmp + strlen(tmp), "%d.%d", HIBYTE(msi->ProcessorRevision), LOBYTE(msi->ProcessorRevision));
                    str = tmp;
                    break;
                case PROCESSOR_ARCHITECTURE_MIPS:
                    str = "Mips";
                    break;
                case PROCESSOR_ARCHITECTURE_ALPHA:
                    str = "Alpha";
                    break;
                case PROCESSOR_ARCHITECTURE_PPC:
                    str = "PowerPC";
                    break;
                case PROCESSOR_ARCHITECTURE_ARM:
                    str = "ARM";
                    break;
                case PROCESSOR_ARCHITECTURE_ARM64:
                    str = "ARM64";
                    break;
                case PROCESSOR_ARCHITECTURE_AMD64:
                    str = "X86_64";
                    break;
                case PROCESSOR_ARCHITECTURE_MSIL:
                    str = "MSIL";
                    break;
                case PROCESSOR_ARCHITECTURE_NEUTRAL:
                    str = "Neutral";
                    break;
                default:
                    str = "???";
                    break;
                }
                printf("  Processor: %s (#%d CPUs)\n", str, msi->NumberOfProcessors);
                switch (msi->MajorVersion)
                {
                case 3:
                    switch (msi->MinorVersion)
                    {
                    case 51: str = "NT 3.51"; break;
                    default: str = "3-????"; break;
                    }
                    break;
                case 4:
                    switch (msi->MinorVersion)
                    {
                    case 0: str = (msi->PlatformId == VER_PLATFORM_WIN32_NT) ? "NT 4.0" : "95"; break;
                    case 10: str = "98"; break;
                    case 90: str = "ME"; break;
                    default: str = "4-????"; break;
                    }
                    break;
                case 5:
                    switch (msi->MinorVersion)
                    {
                    case 0: str = "2000"; break;
                    case 1: str = "XP"; break;
                    case 2:
                        if (msi->ProductType == 1) str = "XP";
                        else if (msi->ProductType == 3) str = "Server 2003";
                        else str = "5-????";
                        break;
                    default: str = "5-????"; break;
                    }
                    break;
                case 6:
                    switch (msi->MinorVersion)
                    {
                    case 0:
                        if (msi->ProductType == 1) str = "Vista";
                        else if (msi->ProductType == 3) str = "Server 2008";
                        else str = "6-????";
                        break;
                    case 1:
                        if (msi->ProductType == 1) str = "Win7";
                        else if (msi->ProductType == 3) str = "Server 2008 R2";
                        else str = "6-????";
                    break;
                case 2:
                    if (msi->ProductType == 1) str = "Win8";
                    else if (msi->ProductType == 3) str = "Server 2012";
                    else str = "6-????";
                    break;
                case 3:
                    if (msi->ProductType == 1) str = "Win8.1";
                    else if (msi->ProductType == 3) str = "Server 2012 R2";
                    else str = "6-????";
                    break;
                default: str = "6-????"; break;
                    }
                    break;
                case 10:
                    switch (msi->MinorVersion)
                    {
                    case 0:
                        if (msi->ProductType == 1) str = "Win10";
                        else str = "10-????";
                        break;
                    default: str = "10-????"; break;
                    }
                    break;
                default: str = "???"; break;
                }
                printf("  Version: Windows %s (%u)\n", str, msi->BuildNumber);
                printf("  PlatformId: %u\n", msi->PlatformId);
                printf("  CSD: %s\n", get_mdmp_str(msi->CSDVersionRva));
                printf("  Reserved1: %u\n", msi->Reserved1);
                if (msi->ProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
                {
                    printf("  x86.VendorId: %.12s\n",
                           (const char*)msi->Cpu.X86CpuInfo.VendorId);
                    printf("  x86.VersionInformation: %x\n", msi->Cpu.X86CpuInfo.VersionInformation);
                    printf("  x86.FeatureInformation: %x\n", msi->Cpu.X86CpuInfo.FeatureInformation);
                    printf("  x86.AMDExtendedCpuFeatures: %x\n", msi->Cpu.X86CpuInfo.AMDExtendedCpuFeatures);
                }
                if (sizeof(MINIDUMP_SYSTEM_INFO) + 4 > dir->Location.DataSize &&
                    msi->CSDVersionRva >= dir->Location.Rva + 4)
                {
                    const char*  code = PRD(dir->Location.Rva + sizeof(MINIDUMP_SYSTEM_INFO), 4);
                    const DWORD* wes;
                    if (code && code[0] == 'W' && code[1] == 'I' && code[2] == 'N' && code[3] == 'E' &&
                        *(wes = (const DWORD*)(code += 4)) >= 3)
                    {
                        /* assume we have wine extensions */
                        printf("  Wine details:\n");
                        printf("    build-id: %s\n", code + wes[1]);
                        printf("    system: %s\n", code + wes[2]);
                        printf("    release: %s\n", code + wes[3]);
                    }
                }
            }
            break;
        case MiscInfoStream:
            if (globals_dump_sect("info"))
            {
                const MINIDUMP_MISC_INFO *mmi = stream;

                printf("Misc Information\n");
                printf("  Size: %u\n", mmi->SizeOfInfo);
                printf("  Flags: %#x\n", mmi->Flags1);
                if (mmi->Flags1 & MINIDUMP_MISC1_PROCESS_ID)
                    printf("  ProcessId: %u\n", mmi->ProcessId);
                if (mmi->Flags1 & MINIDUMP_MISC1_PROCESS_TIMES)
                {
                    printf("  ProcessCreateTime: %s\n", get_time_str(mmi->ProcessCreateTime));
                    printf("  ProcessUserTime: %u\n", mmi->ProcessUserTime);
                    printf("  ProcessKernelTime: %u\n", mmi->ProcessKernelTime);
                }
            }
            break;
        case ExceptionStream:
            if (globals_dump_sect("exception"))
            {
                const MINIDUMP_EXCEPTION_STREAM *mes = stream;

                printf("Stream [%u]: Exception:\n", idx);
                printf("  ThreadId: %#x\n", mes->ThreadId);
                printf("  ExceptionRecord:\n");
                printf("  ExceptionCode: %#x\n", mes->ExceptionRecord.ExceptionCode);
                printf("  ExceptionFlags: %#x\n", mes->ExceptionRecord.ExceptionFlags);
                printf("  ExceptionRecord: %s\n", get_hexint64_str( mes->ExceptionRecord.ExceptionRecord));
                printf("  ExceptionAddress: %s\n", get_hexint64_str( mes->ExceptionRecord.ExceptionAddress));
                printf("  ExceptionNumberParameters: %u\n", mes->ExceptionRecord.NumberParameters);
                for (i = 0; i < mes->ExceptionRecord.NumberParameters; i++)
                {
                    printf("    [%d] %s\n", i, get_hexint64_str(mes->ExceptionRecord.ExceptionInformation[i]));
                }
                printf("  ThreadContext:\n");
                dump_mdmp_data(&mes->ThreadContext, "    ");
            }
            break;
        case HandleDataStream:
            if (globals_dump_sect("handle"))
            {
                const MINIDUMP_HANDLE_DATA_STREAM *mhd = stream;

                printf("Stream [%u]: Handle data:\n", idx);
                printf("  SizeOfHeader: %u\n", mhd->SizeOfHeader);
                printf("  SizeOfDescriptor: %u\n", mhd->SizeOfDescriptor);
                printf("  NumberOfDescriptors: %u\n", mhd->NumberOfDescriptors);

                ptr = (BYTE *)mhd + sizeof(*mhd);
                for (i = 0; i < mhd->NumberOfDescriptors; ++i)
                {
                    const MINIDUMP_HANDLE_DESCRIPTOR_2 *hd = (void *)ptr;

                    printf("  Handle [%u]:\n", i);
                    printf("    Handle: %s\n", get_hexint64_str(hd->Handle));
                    printf("    TypeName: %s\n", get_mdmp_str(hd->TypeNameRva));
                    printf("    ObjectName: %s\n", get_mdmp_str(hd->ObjectNameRva));
                    printf("    Attributes: %#x\n", hd->Attributes);
                    printf("    GrantedAccess: %#x\n", hd->GrantedAccess);
                    printf("    HandleCount: %u\n", hd->HandleCount);
                    printf("    PointerCount: %#x\n", hd->PointerCount);

                    if (mhd->SizeOfDescriptor >= sizeof(MINIDUMP_HANDLE_DESCRIPTOR_2))
                    {
                        printf("    ObjectInfo: %s\n", get_mdmp_str(hd->ObjectInfoRva));
                        printf("    Reserved0: %#x\n", hd->Reserved0);
                    }

                    ptr += mhd->SizeOfDescriptor;
                }
            }
            break;
        case ThreadInfoListStream:
            if (globals_dump_sect("thread"))
            {
                const MINIDUMP_THREAD_INFO_LIST *til = stream;

                printf("Stream [%u]: Thread Info List:\n", idx);
                printf("  SizeOfHeader: %u\n", (UINT)til->SizeOfHeader);
                printf("  SizeOfEntry: %u\n", (UINT)til->SizeOfEntry);
                printf("  NumberOfEntries: %u\n", (UINT)til->NumberOfEntries);

                ptr = (BYTE *)til + sizeof(*til);
                for (i = 0; i < til->NumberOfEntries; ++i)
                {
                    const MINIDUMP_THREAD_INFO *ti = (void *)ptr;

                    printf("  Thread [%u]:\n", i);
                    printf("    ThreadId: %#x\n", ti->ThreadId);
                    printf("    DumpFlags: %#x\n", ti->DumpFlags);
                    printf("    DumpError: %u\n", ti->DumpError);
                    printf("    ExitStatus: %u\n", ti->ExitStatus);
                    printf("    CreateTime: %s\n", get_uint64_str(ti->CreateTime));
                    printf("    ExitTime: %s\n", get_hexint64_str(ti->ExitTime));
                    printf("    KernelTime: %s\n", get_uint64_str(ti->KernelTime));
                    printf("    UserTime: %s\n", get_uint64_str(ti->UserTime));
                    printf("    StartAddress: %s\n", get_hexint64_str(ti->StartAddress));
                    printf("    Affinity: %s\n", get_uint64_str(ti->Affinity));

                    ptr += til->SizeOfEntry;
                }
            }
            break;
        case UnloadedModuleListStream:
            if (globals_dump_sect("module"))
            {
                const MINIDUMP_UNLOADED_MODULE_LIST *uml = stream;

                printf("Stream [%u]: Unloaded module list:\n", idx);
                printf("  SizeOfHeader: %u\n", uml->SizeOfHeader);
                printf("  SizeOfEntry: %u\n", uml->SizeOfEntry);
                printf("  NumberOfEntries: %u\n", uml->NumberOfEntries);

                ptr = (BYTE *)uml + sizeof(*uml);
                for (i = 0; i < uml->NumberOfEntries; ++i)
                {
                    const MINIDUMP_UNLOADED_MODULE *mod = (void *)ptr;

                    printf("  Module [%u]:\n", i);
                    printf("    BaseOfImage: %s\n", get_hexint64_str(mod->BaseOfImage));
                    printf("    SizeOfImage: %u\n", mod->SizeOfImage);
                    printf("    CheckSum: %#x\n", mod->CheckSum);
                    printf("    TimeDateStamp: %s\n", get_time_str(mod->TimeDateStamp));
                    printf("    ModuleName: %s\n", get_mdmp_str(mod->ModuleNameRva));

                    ptr += uml->SizeOfEntry;
                }
            }
            break;
        default:
            printf("Stream [%u]: NIY %d\n", idx, dir->StreamType);
            printf("  RVA: %#x\n", (UINT)dir->Location.Rva);
            printf("  Size: %u\n", dir->Location.DataSize);
            if (globals_dump_sect("content"))
                dump_mdmp_data(&dir->Location, "    ");
            break;
        }
    }
}
