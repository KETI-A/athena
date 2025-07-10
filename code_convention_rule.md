# ATHENA Project Coding Conventions

## Overview
This document defines the coding style guidelines for the ATHENA (KETI SDV Software Defined Vehicle) project.

## 1. Comments

### 1.1 Language Rules
- **All comments must be written in English only.**
- Korean comments are not allowed.

### 1.2 Comment Style
- Single line comments: `/* Comment */`
- Multi-line comments: Use `/* ... */` blocks
- Prefer `/* */` style over `//` style

## 2. Variable Naming

### 2.1 Hungarian Notation Usage
- **All variables must use Hungarian notation.**
- Do not use underscores (`_`).

### 2.2 Prefix Rules

| Prefix | Meaning              | Type                        | Example                        |
|--------|----------------------|-----------------------------|--------------------------------|
| `n`    | Number (Integer)     | `int`, `int32_t`           | `nClientCount`, `nReturnValue` |
| `un`   | Unsigned Number      | `uint32_t`, `unsigned int` | `unDeviceId`, `unPortNumber`   |
| `h`    | Handle               | Handles, pointers          | `hClientMutex`, `hListenSocket`|
| `an`   | Array of Numbers     | Number arrays              | `anClientSockets[MAX_CLIENTS]` |
| `ach`  | Array of Characters  | Character arrays           | `achMsgBuffer[MAX_LINE]`       |
| `sz`   | Size                 | `size_t`                   | `szDataLength`, `szWriteLength`|
| `pv`   | Pointer to Void      | `void*`                    | `pvUserData`, `pvBuffer`       |
| `pn`   | Pointer to Number    | `int*`                     | `pnClientSocket`               |
| `pst`  | Pointer to Struct    | Struct pointers            | `pstMsgManager`                |
| `b`    | Boolean              | `bool`                     | `bIsConnected`, `bProcessComplete`|
| `e`    | Enumeration          | `enum`                     | `eFileType`, `eCommType`       |
| `s_`   | Static Variable      | Static variables           | `s_stMsgManagerTx`, `s_bCliMsgLog`, `s_nClientCount` |

### 2.3 Variable Name Examples

```c
// GOOD: Hungarian notation
int nClientCount = 0;
uint8_t achLineBuffer[MAX_BUFFER_SIZE];
bool bIsConnected = FALSE;
pthread_mutex_t hClientMutex;
size_t szDataLength = 0;

// BAD: Snake case or unclear naming
int client_count = 0;
char line_buffer[MAX_BUFFER_SIZE];
bool connected = false;
pthread_mutex_t client_mutex;
size_t data_len = 0;
```

### 2.4 Static Variable Naming

**All static variables must use `s_` prefix along with their type prefix.**

```c
// GOOD: Static variables with s_ prefix
static int s_nClientCount = 0;
static uint8_t s_achBuffer[MAX_SIZE];
static bool s_bIsRunning = FALSE;
static pthread_mutex_t s_hClientMutex = PTHREAD_MUTEX_INITIALIZER;
static DB_V2X_T s_stDbV2x;

// BAD: Static variables without s_ prefix
static int nClientCount = 0;
static uint8_t achBuffer[MAX_SIZE];
static bool bIsRunning = FALSE;
static pthread_mutex_t hClientMutex = PTHREAD_MUTEX_INITIALIZER;
```

### 2.5 Abbreviations in Variable Names

**Common words in variable names should be abbreviated consistently.**

| Full Word     | Abbreviation | Usage Example             |
|---------------|--------------|---------------------------|
| `Return`      | `Ret`        | `nRetValue`, `nRetCode`   |
| `Result`      | `Res`        | `nResCode`, `nResStatus`  |
| `System`      | `Sys`        | `nSysError`, `hSysHandle` |
| `Count`       | `Cnt`        | `nCntTotal`, `unCntRetry` |
| `Length`      | `Len`        | `nLenBuffer`, `szLenData` |
| `Position`    | `Pos`        | `nPosIndex`, `nPosStart`  |
| `Maximum`     | `Max`        | `nMaxSize`, `unMaxLength` |
| `Minimum`     | `Min`        | `nMinValue`, `nMinBuffer` |
| `Current`     | `Cur`        | `nCurIndex`, `pstCurNode` |
| `Previous`    | `Prev`       | `nPrevValue`, `pstPrevNode` |
| `Next`        | `Next`       | `pstNextNode`, `nNextIndex` |
| `Destination` | `Dst`        | `puchDstBuffer`, `stDstAddr` |
| `Source`      | `Src`        | `puchSrcBuffer`, `stSrcAddr` |
| `Reference`   | `Ref`        | `nRefCount`, `unRefId` |
| `Buffer`      | `Buf`        | `puchBufData`, `stBufInfo` |
| `Message`     | `Msg`        | `stMsgHeader`, `pstMsgData` |
| `Parameter`   | `Param`      | `stParamInfo`, `pstParamList` |
| `Configuration` | `Cfg`      | `stCfgData`, `pstCfgInfo` |

```c
// GOOD: Consistent abbreviations
int32_t nRetCode = FRAMEWORK_OK;
int32_t nResStatus = ProcessData();
uint32_t unMaxSize = GetMaxBufferSize();

// BAD: Inconsistent or full words
int32_t nReturnValue = FRAMEWORK_OK;  // Use nRetValue instead
int32_t nResultStatus = ProcessData(); // Use nResStatus instead
uint32_t unMaximumSize = GetMaxBufferSize(); // Use unMaxSize instead
```

## 3. Data Types

### 3.1 Use Explicit Size Types
- Use `uint8_t` instead of `unsigned char`
- Prefer standard size types

```c
// GOOD: Explicit size types
uint8_t achBuffer[MAX_SIZE];
uint32_t unDeviceId;
int32_t nReturnCode;

// BAD: Non-explicit types
unsigned char buffer[MAX_SIZE];
unsigned int device_id;
int return_code;
```

## 4. Function Structure

### 4.1 Variable Declaration Position
- **All variables must be declared at the beginning of the function.**
- Do not declare variables where they are used.

```c
// GOOD: All variables declared at the top
int32_t SomeFunction(void)
{
    int32_t nRet = FRAMEWORK_ERROR;
    int32_t nLoopCounter = 0;
    size_t szDataLength = 0;
    uint8_t achBuffer[MAX_SIZE];
    bool bProcessComplete = FALSE;
    
    /* Function logic here */
    for (nLoopCounter = 0; nLoopCounter < MAX_COUNT; nLoopCounter++)
    {
        /* ... */
    }
    
    nRet = FRAMEWORK_OK;
    return nRet;
}

// BAD: Variables declared where used
int32_t SomeFunction(void)
{
    for (int i = 0; i < MAX_COUNT; i++)  // BAD: Declaration in loop
    {
        size_t len = strlen(buffer);      // BAD: Declaration in middle
        /* ... */
    }
}
```

### 4.2 Single Return Rule
- **There should be only one return statement per function.**
- **Use nested if-else statements instead of goto for error handling.**

```c
// GOOD: Single return with nested if-else
int32_t SomeFunction(void)
{
    int32_t nRet = FRAMEWORK_ERROR;
    int32_t nResult = 0;
    
    nResult = SomeOperation();
    if (nResult >= 0)
    {
        nResult = AnotherOperation();
        if (nResult >= 0)
        {
            nRet = FRAMEWORK_OK;
        }
        else
        {
            PrintError("Another operation failed");
        }
    }
    else
    {
        PrintError("Operation failed");
    }
    
    return nRet;
}

// BAD: Multiple returns
int32_t SomeFunction(void)
{
    if (SomeOperation() < 0)
    {
        return FRAMEWORK_ERROR;  // BAD: Early return
    }
    
    if (AnotherOperation() < 0)
    {
        return FRAMEWORK_ERROR;  // BAD: Multiple returns
    }
    
    return FRAMEWORK_OK;
}
```

## 5. Code Formatting

### 5.1 Brace Style
- **Place braces on the next line.**

```c
// GOOD: Braces on new line
if (condition)
{
    /* code here */
}

while (nLoopCounter < MAX_COUNT)
{
    /* code here */
    nLoopCounter++;
}

// BAD: Same line braces
if (condition) {
    /* code here */
}
```

### 5.2 Indentation
- **Use 4 spaces instead of tabs.**
- Maintain consistent indentation.

```c
// GOOD: 4 spaces indentation
if (condition)
{
    if (another_condition)
    {
        DoSomething();
    }
}

// BAD: Tab or inconsistent spacing
if (condition)
{
	if (another_condition)  // TAB used
	{
		DoSomething();
	}
}
```

### 5.3 Whitespace Handling
- **Remove unnecessary whitespace at line endings.**
- **Remove unnecessary whitespace after newlines.**
- Use appropriate spacing around operators.

```c
// GOOD: Proper spacing and no trailing whitespace
nResult = nValue1 + nValue2;
if (nCounter >= MAX_VALUE)
{
    ProcessValue();
}

// BAD: Inconsistent spacing
nResult=nValue1+nValue2;
if(nCounter>=MAX_VALUE)

// BAD: Trailing whitespace after newline
int nValue = 0;    
    /* <- This line has unnecessary whitespace */

// GOOD: No trailing whitespace
int nValue = 0;
/* <- This line has no unnecessary whitespace */
```

### 5.4 Conditional Statement Style
- **Always use parentheses for nested conditions.**
- **One space between if and the opening parenthesis.**
- **One space between closing parenthesis and opening brace.**

```c
// GOOD: Proper conditional formatting with nested parentheses
if ((nValue > MIN_VALUE) && (nValue < MAX_VALUE))
{
    ProcessValue();
}

if ((bIsConnected == TRUE) || (nRetryCount > MAX_RETRIES))
{
    CloseConnection();
}

// BAD: Missing parentheses for nested conditions
if (nValue > MIN_VALUE && nValue < MAX_VALUE)
{
    ProcessValue();
}

// BAD: No space between if and parenthesis
if(bIsConnected == TRUE)
{
    ProcessValue();
}

// BAD: No space between closing parenthesis and brace
if (nValue > MIN_VALUE){
    ProcessValue();
}
```

## 6. Constants and Macros

### 6.1 Macro Naming
- **Macros must have file name as prefix.**
- **All macro names should be uppercase.**

```c
// GOOD: Macros with file name prefix
// In file: cli_msg.c
#define CLI_MSG_MAX_SAFE_WRITE_LENGTH 500
#define CLI_MSG_MAX_CLIENTS 100

// In file: db_manager.c
#define DB_MANAGER_DEFAULT_PORT 8080
#define DB_MANAGER_MAX_TIMEOUT 3000

// BAD: Macros without file name prefix
#define MAX_SAFE_WRITE_LENGTH 500
#define MAX_CLIENTS 100
#define DEFAULT_PORT 8080
```

### 6.2 No Magic Numbers
- **Define meaningful constants instead of hardcoded numbers.**

```c
// GOOD: Named constants
#define CLI_MSG_MAX_SAFE_WRITE_LENGTH 500
#define CLI_MSG_MAX_CLIENTS 100
#define DB_MANAGER_DEFAULT_PORT 8080

if (szWriteLength > CLI_MSG_MAX_SAFE_WRITE_LENGTH)
{
    szWriteLength = CLI_MSG_MAX_SAFE_WRITE_LENGTH;
}

// BAD: Magic numbers
if (write_len > 500)  // What does 500 mean?
{
    write_len = 500;
}
```

### 6.2 Constant Naming Rules
- Use all uppercase letters
- Separate words with underscores

```c
#define MAX_BUFFER_SIZE 1024
#define MSG_MANAGER_TIMEOUT 5000
#define CLI_SUCCESS_CODE 0
```

## 7. Error Handling

### 7.1 Error Code Management
- Store error codes in `nRet` variable instead of immediate return
- **Use nested if-else statements for resource management and cleanup**

```c
// GOOD: Proper error handling with nested if-else
int32_t ProcessData(void)
{
    int32_t nRet = FRAMEWORK_ERROR;
    FILE *hFile = NULL;
    uint8_t *puchBuffer = NULL;
    
    hFile = fopen("data.txt", "r");
    if (hFile != NULL)
    {
        puchBuffer = malloc(MAX_BUFFER_SIZE);
        if (puchBuffer != NULL)
        {
            /* Process data */
            nRet = FRAMEWORK_OK;
            free(puchBuffer);
        }
        else
        {
            PrintError("Memory allocation failed");
            nRet = MEMORY_ALLOCATION_ERROR;
        }
        fclose(hFile);
    }
    else
    {
        PrintError("Failed to open file");
        nRet = FILE_OPEN_ERROR;
    }
    
    return nRet;
}
```

### 7.2 Code Flow Control
- **Do not use `goto` statements.**
- **Prefer using `if-else` constructs to control code flow.**
- **Use a single return statement at the end of the function when possible.**

```c
// GOOD: Using if-else for flow control with single return
int32_t ProcessData(uint8_t *puchData, uint32_t unLength)
{
    int32_t nRet = FRAMEWORK_ERROR;
    
    if (puchData == NULL || unLength == 0)
    {
        PrintError("Invalid parameters");
    }
    else if (unLength > MAX_DATA_LENGTH)
    {
        PrintError("Data length exceeds maximum allowed");
    }
    else
    {
        // Process data
        nRet = FRAMEWORK_OK;
    }
    
    return nRet;
}

// BAD: Using multiple return statements
int32_t ProcessData(uint8_t *puchData, uint32_t unLength)
{
    if (puchData == NULL || unLength == 0)
    {
        PrintError("Invalid parameters");
        return FRAMEWORK_ERROR;
    }
    
    if (unLength > MAX_DATA_LENGTH)
    {
        PrintError("Data length exceeds maximum allowed");
        return FRAMEWORK_ERROR;
    }
    
    // Process data
    return FRAMEWORK_OK;
}

// GOOD: Resource cleanup with nested if-else
int32_t ProcessFile(const char *pchFilename)
{
    int32_t nRet = FRAMEWORK_ERROR;
    FILE *hFile = NULL;
    uint8_t *puchBuffer = NULL;
    
    if (pchFilename != NULL)
    {
        hFile = fopen(pchFilename, "r");
        if (hFile != NULL)
        {
            puchBuffer = malloc(BUFFER_SIZE);
            if (puchBuffer != NULL)
            {
                // Process file
                nRet = FRAMEWORK_OK;
                free(puchBuffer);
            }
            else
            {
                PrintError("Memory allocation failed");
            }
            fclose(hFile);
        }
        else
        {
            PrintError("Failed to open file");
        }
    }
    else
    {
        PrintError("Invalid filename");
    }
    
    return nRet;
}
```