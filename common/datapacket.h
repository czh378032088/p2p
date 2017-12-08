
#ifndef __DATAPACKET__
#define __DATAPACKET__

#include "typedefine.h"

enum PacketType
{
    CL_TO_CL = 0,
    CL_TO_SV,
    SV_TO_CL,
    CN_TO_CN,
    CN_TO_SV,
    SV_TO_CN,
    NO_TO_SV,
    SV_TO_NO,
    INVALID_TYPE
};

enum CmdType
{
    NoneCmd = 0,
    RelDataCmd,
    ReturnCmd,
    RegClCmd,
    ReqClIdCmd,
    RegCnCmd,
    ReqCnCmd,
    ReqIpCmd,
    ReqResendCmd,
    InvalidCmd
};

enum ErrorCode
{
    NoError = 0,
    PacketTypeInvalidError = -1,
    PacketLenError = -2,
    CmdInvalidError = -3,
    NoDisClientError = -4,
    NoDisConnectError = -5,
    NoSrcClientError = -6,
    NoSrcConnectError = -7,
    CmdNotSupportError = -8,
    PacketTypeNotSupportError = -9
};

using namespace std;

class DataPacket
{
public:
    DataPacket(uint8_t buff[],int length);
    ~DataPacket();
    static const int s_minPacketSize;
    PacketType GetPacketType(void);
    CmdType GetPacketCmd(void);
    CmdType GetPacketRetCmd(void);
    uint32_t GetSourceId(void);
    uint32_t GetDestinatId(void);
    int  GetChannel(void);
    uint8_t GetSerialNum(void);
    int  GetSrcConnectId(void);
    int  GetDisConnectId(void);
    int  GetLength(void);
    int  GetErrorCode(void);
    uint8_t *GetDataBuff(void);
    char *GetClentName(void);

    void SetPacketType(PacketType type);
    void SetPacketCmd(CmdType cmd);
    void SetPacketRetCmd(CmdType cmd);
    void SetSourceId(uint32_t id);
    void SetDestinatId(uint32_t id);
    void SetChannel(int channel);
    void SetSerialNum(uint8_t num);
    void SetConnectId(int sId,int dId);
    void SetErrorCode(int errorcode);
    void UpdateLength(int len);
    void SetClentName(const char *name);

    int PushData(uint8_t buff[],int size);
    int PopData(uint8_t buff[],int size);
    uint8_t *GetDataField(void);
    int GetDataFieldSize(void);
    int GetPacketHeadSize(void);
    int GetResendNum(uint8_t buff[]);

    int GenErrorRetPacket(DataPacket &srcPacket,ErrorCode error);
    int GenAutoErrorRetPacket(DataPacket &srcPacket);
    int GenStandardRetPacket(DataPacket &srcPacket);
    int GenRegClPacket(const char buff[]);
    int GenRegCnPacket(uint32_t clientId);
    int GenNonePacket(uint32_t srcId,uint32_t disId);
    int GenGetIPPacket(void);
    int GenReqClientIdPacket(uint32_t srcId,uint8_t srcConnect,const char buff[]);
    int GenReqConnectPacket(uint32_t srcId,uint32_t disId,uint8_t srcCn,uint8_t disCn,uint8_t cnMode,uint8_t addr[],int addrLen);
    int GenReqConnectPacket1(uint32_t srcId,uint32_t disId,uint8_t srcCn,uint8_t disCn,uint8_t cnMode,uint8_t addr[],int addrLen);
    int GenReplayPacket(uint32_t srcId,uint32_t disId,uint8_t channel);
    int GenReSendPacket(uint32_t srcId,uint32_t disId,uint8_t channel,uint8_t resendBuff[],int resendSize);
protected:
    PacketType SetRetPacketType(PacketType srcType);
private:
    uint8_t *m_pdata;
    int   m_dataLen;
    int   m_popIndex;
};

#endif
