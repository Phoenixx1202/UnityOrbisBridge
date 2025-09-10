#include "../headers/includes.hpp"

const char *logPrefixes[] =
    {"[DEBUG]", "[INFO]",
     "[WARNING]",
     "[ERROR]",
     "[CRITICAL]"};

uint16_t inputTextBuffer[513];
uint16_t inputImeTitle[512];

extern int main() { return 1337; };

void printToConsole(int type, const char *message, ...)
{
  if (type < 0 || type > 4)
    type = 0;

  const size_t bufferSize = 1024;
  char buffer[bufferSize];

  va_list args;
  va_start(args, message);
  std::vsnprintf(buffer, bufferSize, message, args);
  va_end(args);

  std::string logMessage = std::string(logPrefixes[type]) + " " + buffer;
  sceKernelDebugOutText(0, (logMessage + "\n").c_str());
}

void printAndLog(int type, const char *message, ...)
{
  if (type < 0 || type > 4)
    type = 0;

  const size_t bufferSize = 1024;
  char buffer[bufferSize];

  va_list args;
  va_start(args, message);
  std::vsnprintf(buffer, bufferSize, message, args);
  va_end(args);

  auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  char timestamp[100];
  std::strftime(timestamp, sizeof(timestamp), "%m/%d/%Y @ %I:%M:%S%p", std::localtime(&now));

  std::string logMessage = std::string(timestamp) + " " + logPrefixes[type] + " " + buffer;
  printToConsole(type, "%s", buffer);
  AppendFile(logMessage.c_str());
}

int convert_to_utf16(const char *utf8, uint16_t *utf16, uint32_t available)
{
  int count = 0;
  while (*utf8)
  {
    uint8_t ch = (uint8_t)*utf8++;
    uint32_t code, extra;

    if (ch < 0x80)
    {
      code = ch;
      extra = 0;
    }
    else if ((ch & 0xe0) == 0xc0)
    {
      code = ch & 31;
      extra = 1;
    }
    else if ((ch & 0xf0) == 0xe0)
    {
      code = ch & 15;
      extra = 2;
    }
    else
    {
      code = ch & 7;
      extra = 3;
    }

    for (uint32_t i = 0; i < extra; i++)
    {
      uint8_t next = (uint8_t)*utf8++;

      if (next == 0 || (next & 0xc0) != 0x80)
        goto utf16_end;

      code = (code << 6) | (next & 0x3f);
    }

    if (code < 0xd800 || code >= 0xe000)
    {
      if (available < 1)
        goto utf16_end;

      utf16[count++] = (uint16_t)code;

      available--;
    }
    else
    {
      if (available < 2)
        goto utf16_end;

      code -= 0x10000;

      utf16[count++] = 0xd800 | (code >> 10);
      utf16[count++] = 0xdc00 | (code & 0x3ff);

      available -= 2;
    }
  }

utf16_end:
  utf16[count] = 0;

  return count;
}

int convert_from_utf16(const uint16_t *utf16, char *utf8, uint32_t size)
{
  int count = 0;

  while (*utf16)
  {
    uint32_t code;
    uint16_t ch = *utf16++;

    if (ch < 0xd800 || ch >= 0xe000)
      code = ch;
    else
    {
      uint16_t ch2 = *utf16++;
      if (ch < 0xdc00 || ch > 0xe000 || ch2 < 0xd800 || ch2 > 0xdc00)
        goto utf8_end;

      code = 0x10000 + ((ch & 0x03FF) << 10) + (ch2 & 0x03FF);
    }

    if (code < 0x80)
    {
      if (size < 1)
        goto utf8_end;

      utf8[count++] = (char)code;

      size--;
    }
    else if (code < 0x800)
    {
      if (size < 2)
        goto utf8_end;

      utf8[count++] = (char)(0xc0 | (code >> 6));
      utf8[count++] = (char)(0x80 | (code & 0x3f));

      size -= 2;
    }
    else if (code < 0x10000)
    {
      if (size < 3)
        goto utf8_end;

      utf8[count++] = (char)(0xe0 | (code >> 12));
      utf8[count++] = (char)(0x80 | ((code >> 6) & 0x3f));
      utf8[count++] = (char)(0x80 | (code & 0x3f));

      size -= 3;
    }
    else
    {
      if (size < 4)
        goto utf8_end;

      utf8[count++] = (char)(0xf0 | (code >> 18));
      utf8[count++] = (char)(0x80 | ((code >> 12) & 0x3f));
      utf8[count++] = (char)(0x80 | ((code >> 6) & 0x3f));
      utf8[count++] = (char)(0x80 | (code & 0x3f));

      size -= 4;
    }
  }

utf8_end:
  utf8[count] = 0;

  return count;
}

bool getKeyboardInput(const char *Title, const char *initialTextBuffer, char *out_buffer)
{
  if (sceSysmoduleLoadModule(ORBIS_SYSMODULE_IME_DIALOG) != 0)
    return false;

  if (initialTextBuffer && strlen(initialTextBuffer) > 254)
    return false;

  memset(inputTextBuffer, 0, sizeof(inputTextBuffer));
  memset(inputImeTitle, 0, sizeof(inputImeTitle));

  if (initialTextBuffer)
  {
    strncpy(out_buffer, initialTextBuffer, 254);
    convert_to_utf16(initialTextBuffer, inputTextBuffer, sizeof(inputTextBuffer));
  }

  if (Title)
    convert_to_utf16(Title, inputImeTitle, sizeof(inputImeTitle));

  OrbisImeDialogSetting param = {};
  param.maxTextLength = 254;
  param.inputTextBuffer = reinterpret_cast<wchar_t *>(inputTextBuffer);
  param.title = reinterpret_cast<wchar_t *>(inputImeTitle);
  param.userId = 0xFE;
  param.type = ORBIS_TYPE_DEFAULT;
  param.enterLabel = ORBIS_BUTTON_LABEL_DEFAULT;

  if (sceImeDialogInit(&param, nullptr) != 0)
    return false;

  while (true)
  {
    if (sceImeDialogGetStatus() == ORBIS_DIALOG_STATUS_STOPPED)
    {
      OrbisDialogResult result = {};
      sceImeDialogGetResult(&result);

      if (result.endstatus == ORBIS_DIALOG_OK)
      {
        convert_from_utf16(inputTextBuffer, out_buffer, sizeof(inputTextBuffer));
        sceImeDialogTerm();

        return true;
      }
      break;
    }
    else if (sceImeDialogGetStatus() == ORBIS_DIALOG_STATUS_NONE)
      break;
  }

  sceImeDialogTerm();
  return false;
}

int df(std::string mountPoint, long &percentUsed, double &totalSpace, double &usedSpace, double &freeSpace)
{
  struct statfs s;
  long blocks_used = 0;
  long blocks_percent_used = 0;

  if (SYSCALL(396, mountPoint.c_str(), &s) != 0)
  {
    PrintToConsole(("df cannot open " + mountPoint).c_str(), 2);

    return 0;
  }

  if (s.f_blocks > 0)
  {
    blocks_used = s.f_blocks - s.f_bfree;
    blocks_percent_used = (long)(blocks_used * 100.0 / (blocks_used + s.f_bavail) + 0.5);
  }

  totalSpace = s.f_blocks * (s.f_bsize / 1024.0 / 1024.0 / 1024.0);
  freeSpace = s.f_bavail * (s.f_bsize / 1024.0 / 1024.0 / 1024.0);
  usedSpace = totalSpace - freeSpace;

  percentUsed = blocks_percent_used;

  return blocks_percent_used;
}

void build_iovec(struct iovec **iov, int *iovlen, const char *name, const void *val, size_t len)
{
  int i = *iovlen;

  *iov = (struct iovec *)realloc(*iov, sizeof(struct iovec) * (i + 2));

  if (*iov == NULL)
  {
    *iovlen = -1;
    return;
  }

  (*iov)[i].iov_base = strdup(name);
  (*iov)[i].iov_len = strlen(name) + 1;

  ++i;

  (*iov)[i].iov_base = (void *)val;

  if (len == (size_t)-1)
    len = val ? strlen((const char *)val) + 1 : 0;

  (*iov)[i].iov_len = (int)len;

  *iovlen = ++i;
}

void mount_large_fs(const char *device, const char *mountpoint, const char *fstype, const char *mode, unsigned int flags)
{
  struct iovec *iov = NULL;
  int iovlen = 0;

  build_iovec(&iov, &iovlen, "fstype", fstype, -1);
  build_iovec(&iov, &iovlen, "fspath", mountpoint, -1);
  build_iovec(&iov, &iovlen, "from", device, -1);
  build_iovec(&iov, &iovlen, "large", "yes", -1);
  build_iovec(&iov, &iovlen, "timezone", "static", -1);
  build_iovec(&iov, &iovlen, "async", "", -1);
  build_iovec(&iov, &iovlen, "ignoreacl", "", -1);

  if (mode)
  {
    build_iovec(&iov, &iovlen, "dirmask", mode, -1);
    build_iovec(&iov, &iovlen, "mask", mode, -1);
  }

  syscall(378, iov, iovlen, flags);
}

bool if_exists(const char *path)
{
  struct stat buffer;
  return (stat(path, &buffer) == 0);
}

int initiateProgressDialog(const char *format, ...)
{
  char buff[1024] = {};
  va_list args;
  va_start(args, format);
  vsprintf(buff, format, args);
  va_end(args);

  sceSysmoduleLoadModule(ORBIS_SYSMODULE_MESSAGE_DIALOG);
  sceMsgDialogInitialize();

  OrbisMsgDialogParam dialogParam = {};
  OrbisMsgDialogParamInitialize(&dialogParam);
  dialogParam.mode = ORBIS_MSG_DIALOG_MODE_PROGRESS_BAR;

  OrbisMsgDialogProgressBarParam progBarParam = {};
  dialogParam.progBarParam = &progBarParam;
  dialogParam.progBarParam->barType = ORBIS_MSG_DIALOG_PROGRESSBAR_TYPE_PERCENTAGE;
  dialogParam.progBarParam->msg = buff;

  sceMsgDialogOpen(&dialogParam);
  return 0;
}

void setProgressMsgText(int prog, const char *fmt, ...)
{
  char buff[300] = {};
  va_list args;
  va_start(args, fmt);
  vsnprintf(buff, sizeof(buff) - 1, fmt, args);
  va_end(args);

  sceMsgDialogProgressBarSetValue(0, prog);
  sceMsgDialogProgressBarSetMsg(0, buff);
}

int showDialogMessage(char *format, ...)
{
  InitializeNativeDialogs();

  char buff[1024];
  memset(&buff[0], 0, 1024);

  va_list args;
  va_start(args, format);
  vsprintf(&buff[0], format, args);
  va_end(args);

  OrbisMsgDialogButtonsParam buttonsParam;
  OrbisMsgDialogUserMessageParam messageParam;
  OrbisMsgDialogParam dialogParam;

  OrbisMsgDialogParamInitialize(&dialogParam);

  memset(&buttonsParam, 0x00, sizeof(buttonsParam));
  memset(&messageParam, 0x00, sizeof(messageParam));

  dialogParam.userMsgParam = &messageParam;
  dialogParam.mode = ORBIS_MSG_DIALOG_MODE_USER_MSG;

  messageParam.buttonType = ORBIS_MSG_DIALOG_BUTTON_TYPE_WAIT;

  messageParam.msg = buff;

  sceMsgDialogOpen(&dialogParam);

  return 0;
}
