#include "datapacket.h"
#include "debug.h"
#include <string.h>

const int DataPacket::s_minPacketSize = 12;

DataPacket::DataPacket(uint8_t buff[],int length)
{
    m_pdata = buff;
    m_dataLen = length;
    m_popIndex = 0;
}

DataPacket::~DataPacket()
{
}

PacketType DataPacket::GetPacketType(void)
{
    uint8_t temp1 = ((~m_pdata[0]) >> 4) & 0x0f ;
    uint8_t temp2 = m_pdata[0] & 0x0f ;
    //Debug_Printf("temp1 = %d,temp2 = %d\n",temp1,temp2);
    if(temp1 != temp2 || temp1 > INVALID_TYPE)
        return INVALID_TYPE;
    return  (PacketType)temp1;
}

CmdType DataPacket::GetPacketCmd(void)
{
    if(m_pdata[1] >= InvalidCmd)
        return InvalidCmd;
    return (CmdType)m_pdata[1];
}

CmdType DataPacket::GetPacketRetCmd(void)
{
    if(m_pdata[3] >= InvalidCmd)
        return InvalidCmd;
    return (CmdType)m_pdata[3];
}

uint32_t DataPacket::GetSourceId(void)
{
    uint32_t ret ;
    memcpy(&ret,m_pdata + 4,4);
    //Debug_Printf("GetSourceId=%d\n",ret);
    return ret;
}
uint32_t DataPacket::GetDestinatId(void)
{
    uint32_t ret ;
    memcpy(&ret,m_pdata + 8,4);
    return ret;
}

int  DataPacket::GetChannel(void)
{
    return m_pdata[2];
}  

uint8_t DataPacket::GetSerialNum(void)
{
    return m_pdata[3];
}

int  DataPacket::GetSrcConnectId(void)
{
    return m_pdata[2] & 0x0f;
}

int  DataPacket::GetDisConnectId(void)
{
    return (m_pdata[2] >> 4) & 0x0f;
}


int  DataPacket::GetLength(void)
{
    return m_dataLen;
}

int  DataPacket::GetErrorCode(void)
{
    int ret;
    memcpy(&ret,m_pdata + 12,4);
    return ret;
}

uint8_t *DataPacket::GetDataBuff(void)
{
    return m_pdata;
}

char *DataPacket::GetClentName(void)
{
    return (char*)(m_pdata + 12);
}

void DataPacket::SetPacketType(PacketType type)
{
    m_pdata[0] = ((~type) << 4) | type;
}

void DataPacket::SetPacketCmd(CmdType cmd)
{
    m_pdata[1] = cmd;
}

void DataPacket::SetPacketRetCmd(CmdType cmd)
{
    m_pdata[3] = cmd;
}

void DataPacket::SetSourceId(uint32_t id)
{
    memcpy(m_pdata + 4, &id,4);
}

void DataPacket::SetDestinatId(uint32_t id)
{
    memcpy(m_pdata + 8, &id,4);
}

void DataPacket::SetChannel(int channel)
{
    m_pdata[2] = channel;
}

void DataPacket::SetSerialNum(uint8_t num)
{
    m_pdata[3] = num;
}

void DataPacket::SetConnectId(int sId,int dId)
{
    m_pdata[2] = sId | (dId << 4);
}

void DataPacket::SetErrorCode(int errorcode)
{
    memcpy(m_pdata + 12,&errorcode,4);
}

void DataPacket::UpdateLength(int len)
{
    m_dataLen = len;
    m_popIndex = 0;
}

void DataPacket::SetClentName(const char *name)
{
    int i = 0;
    uint8_t *p  = (uint8_t *)name;
    while(name[i])
    {
        m_pdata[12 + i] = p[i];
        i ++;
    }
    m_pdata[i] = 0;
    m_dataLen = 12 + i + 1;
}

int DataPacket::GenErrorRetPacket(DataPacket &srcPacket,ErrorCode error)
{
    SetRetPacketType(srcPacket.GetPacketType());
    SetPacketCmd(ReturnCmd);
    SetPacketRetCmd(srcPacket.GetPacketCmd());
    SetErrorCode(error);
    m_dataLen = 16;
    return m_dataLen;
}

int DataPacket::GenAutoErrorRetPacket(DataPacket &srcPacket)
{  
    m_dataLen = 0;
    if(srcPacket.m_dataLen < s_minPacketSize)
    {
        Debug_Printf("PacketLenError\n");
        SetErrorCode(PacketLenError);
        m_dataLen = 16;
    }
    else if(srcPacket.GetPacketType() == INVALID_TYPE)
    {
        Debug_Printf("PacketTypeInvalidError");
        SetErrorCode(PacketTypeInvalidError);
        m_dataLen = 16;
    }
    else if(srcPacket.GetPacketCmd() == InvalidCmd)
    {
        Debug_Printf("CmdInvalidError");
        SetErrorCode(CmdInvalidError);
        m_dataLen = 16;
    }

    if(m_dataLen > 0)
    {
        SetPacketType(SV_TO_NO);
        SetPacketCmd(ReturnCmd);
        SetPacketRetCmd(srcPacket.GetPacketCmd());
    }
    return m_dataLen;
}

int DataPacket::GenStandardRetPacket(DataPacket &srcPacket)
{
    m_dataLen = 0;
    SetRetPacketType(srcPacket.GetPacketType());
    SetSourceId(srcPacket.GetDestinatId());
    SetDestinatId(srcPacket.GetSourceId());
    //SetConnectId(srcPacket.GetConnectId());
    SetPacketCmd(ReturnCmd);
    SetPacketRetCmd(srcPacket.GetPacketCmd());
    SetErrorCode(NoError);
    m_dataLen = 16;
    return 16;
}

int DataPacket::GenRegClPacket(const char buff[])
{
    m_dataLen = 0;
    SetPacketType(NO_TO_SV);
    SetSourceId(0);
    SetDestinatId(0);
    SetPacketCmd(RegClCmd);
    SetClentName(buff);
    return m_dataLen;
}

int DataPacket::GenRegCnPacket(uint32_t clientId)
{
    m_dataLen = 0;
    SetPacketType(NO_TO_SV);
    SetSourceId(clientId);
    SetDestinatId(0);
    SetPacketCmd(RegCnCmd);
    m_dataLen = 12;
    return m_dataLen;
}

int DataPacket::GenNonePacket(uint32_t srcId,uint32_t disId)
{
    m_dataLen = 0;
    SetPacketType(CL_TO_SV);
    SetSourceId(srcId);
    SetDestinatId(disId);
    SetPacketCmd(NoneCmd);
    m_dataLen = 12;
    return m_dataLen;
}

int DataPacket::GenGetIPPacket(void)
{
    m_dataLen = 0;
    SetPacketType(NO_TO_SV);
    SetSourceId(0);
    SetDestinatId(0);
    SetPacketCmd(ReqIpCmd);
    m_dataLen = 12;
    return m_dataLen;
}

int DataPacket::GenReqClientIdPacket(uint32_t srcId,uint8_t srcConnect,const char buff[])
{
    m_dataLen = 0;
    SetPacketType(CL_TO_SV);
    SetSourceId(srcId);
    SetDestinatId(0);
    SetConnectId(srcConnect,0);
    SetPacketCmd(ReqClIdCmd);
    SetClentName(buff);
    return m_dataLen;
}

int DataPacket::GenReqConnectPacket(uint32_t srcId,uint32_t disId,uint8_t srcCn,uint8_t disCn,uint8_t cnMode,uint8_t addr[],int addrLen)
{
    m_dataLen = 0;
    SetPacketType(CL_TO_CL);
    //Debug_Printf("m_pdata[0] = %d\n",m_pdata[0]);
    SetSourceId(srcId);
    //Debug_Printf("m_pdata[0] = %d\n",m_pdata[0]);
    SetDestinatId(disId);
    //Debug_Printf("m_pdata[0] = %d\n",m_pdata[0]);
    SetConnectId(srcCn,disCn);
    //Debug_Printf("m_pdata[0] = %d\n",m_pdata[0]);
    SetPacketCmd(ReqCnCmd);
    m_dataLen = 12;
    PushData(&cnMode,1);
    //Debug_Printf("m_pdata[0] = %d\n",m_pdata[0]);
    PushData(addr,addrLen);
    //Debug_Printf("m_pdata[0] = %d\n",m_pdata[0]);

    return m_dataLen;
}

int DataPacket::GenReqConnectPacket1(uint32_t srcId,uint32_t disId,uint8_t srcCn,uint8_t disCn,uint8_t cnMode,uint8_t addr[],int addrLen)
{
    m_dataLen = 0;
    SetPacketType(CN_TO_CN);
    //Debug_Printf("m_pdata[0] = %d\n",m_pdata[0]);
    SetSourceId(srcId);
    //Debug_Printf("m_pdata[0] = %d\n",m_pdata[0]);
    SetDestinatId(disId);
    //Debug_Printf("m_pdata[0] = %d\n",m_pdata[0]);
    SetConnectId(srcCn,disCn);
    //Debug_Printf("m_pdata[0] = %d\n",m_pdata[0]);
    SetPacketCmd(ReqCnCmd);
    m_dataLen = 12;
    PushData(&cnMode,1);
    //Debug_Printf("m_pdata[0] = %d\n",m_pdata[0]);
    PushData(addr,addrLen);
    //Debug_Printf("m_pdata[0] = %d\n",m_pdata[0]);

    return m_dataLen;
}

int DataPacket::GenReplayPacket(uint32_t srcId,uint32_t disId,uint8_t channel)
{
    SetPacketType(CN_TO_CN);
    //Debug_Printf("m_pdata[0] = %d\n",m_pdata[0]);
    SetSourceId(srcId);
    //Debug_Printf("m_pdata[0] = %d\n",m_pdata[0]);
    SetDestinatId(disId);
    SetPacketCmd(RelDataCmd);
    SetChannel(channel);
    m_dataLen = 12;
    return m_dataLen;
}

int DataPacket::GenReSendPacket(uint32_t srcId,uint32_t disId,uint8_t channel,uint8_t resendBuff[],int resendSize)
{
    SetPacketType(CN_TO_CN);
    SetSourceId(srcId);
    //Debug_Printf("m_pdata[0] = %d\n",m_pdata[0]);
    SetDestinatId(disId);
    SetPacketCmd(ReqResendCmd);
    SetChannel(channel);
    m_dataLen = 12;
    PushData(resendBuff,resendSize);
    return m_dataLen;
}

int DataPacket::PushData(uint8_t buff[],int size)
{
    memcpy(m_pdata + m_dataLen,buff,size);
    m_dataLen += size;
    return m_dataLen;
}

int DataPacket::PopData(uint8_t buff[],int size)
{
    if(m_popIndex == 0)
    {
        if(GetPacketCmd() == ReturnCmd)
        {
            m_popIndex = 16;
        }
        else 
        {
            m_popIndex = 12;
        }
    }
    //Debug_Printf("m_popIndex = %d\n",m_popIndex);
    memcpy(buff,m_pdata + m_popIndex,size);
    m_popIndex += size;
    return m_popIndex;
}

uint8_t *DataPacket::GetDataField(void)
{
    return m_pdata + 12;
}

int DataPacket::GetDataFieldSize(void)
{
    return m_dataLen - 12;
}

int DataPacket::GetPacketHeadSize(void)
{
    if(m_pdata[1] == ReturnCmd)
    {
        return 16;
    }
    else
    {
        return 12;
    }
}

int DataPacket::GetResendNum(uint8_t buff[])
{
    int len = m_dataLen - 12;
    memcpy(buff,m_pdata + 12,len);
    return len;
}

PacketType DataPacket::SetRetPacketType(PacketType srcType)
{
    PacketType type = INVALID_TYPE;
    if(srcType == CL_TO_CL || srcType == CL_TO_SV)
    {
        type = SV_TO_CL;
    }
    else if(srcType == CN_TO_CN || srcType == CN_TO_SV)
    {
        type = SV_TO_CN;
    }
    else 
    {
        type =  SV_TO_NO;
    }
    SetPacketType(type);
    return type;
}
