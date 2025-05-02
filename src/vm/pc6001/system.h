/*
	NEC PC-6001 Emulator 'yaPC-6001'

	Author : tanam
	Date   : 2013.07.15-

	[ system port ]
*/

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"
#include "../../fileio.h"
#include "d88.h"

// 最大ドライブ接続数
#define	MAXDRV	4

// コマンド
// 実際には PC-80S31 のコマンドだけど大体同じ？
enum FddCommand
{
	INIT				= 0x00,
	WRITE_DATA			= 0x01,
	READ_DATA			= 0x02,
	SEND_DATA			= 0x03,
	COPY				= 0x04,
	FORMAT				= 0x05,
	SEND_RESULT_STATUS	= 0x06,
	SEND_DRIVE_STATUS	= 0x07,
	TRANSMIT			= 0x11,
	RECEIVE				= 0x12,
	LOAD				= 0x14,
	SAVE				= 0x15,
	
	IDLE				= 0xff,	// 処理待ちの状態
	EndofFdcCmd
};

enum FdcPhase {
	IDLEP = 0,
	C_PHASE,
	E_PHASE,
	R_PHASE
};

enum FdcSeek{
	SK_STOP = 0,	// シークなし
	SK_SEEK,		// シーク中
	SK_END			// シーク完了
};

struct PD765 {
	BYTE command;		// コマンド
	
FdcPhase phase;		// Phase (C/E/R)
int step;			// Phase内の処理手順

	BYTE SRT;			// Step Rate Time
	BYTE HUT;			// Head Unloaded Time
	BYTE HLT;			// Head Load Time
	bool ND;			// Non DMA Mode  true:Non DMA false:DMA
	
	FdcSeek SeekSta[4];	// シーク状態
	BYTE NCN[4];		// New Cylinder Number
	BYTE PCN[4];		// Present Cylinder Number
	
	
	BYTE MT;			// Multi-track
	BYTE MF;			// MFM/FM Mode
	BYTE SK;			// Skip
	BYTE HD;			// Head
	BYTE US;			// Unit Select
	
	BYTE C;				// Cylinder Number
	BYTE H;				// Head Address
	BYTE R;				// Record
	BYTE N;				// Number
	BYTE EOT;			// End of Track
	BYTE GPL;			// Gap Length
	BYTE DTL;			// Data length
	
	BYTE D;				// Format Data
	BYTE SC;			// Sector
	
	BYTE st0;			// ST0
	BYTE st1;			// ST1
	BYTE st2;			// ST2
	BYTE st3;			// ST3
	
	BYTE status;		// Status
	bool intr;			// FDC割込み発生フラグ
	
		PD765() :
		command(0), phase(R_PHASE), step(0),
		SRT(32), HUT(0), HLT(0), ND(false),
		MT(0), MF(0), SK(0), HD(0), US(0), C(0), H(0), R(0), N(0),
		EOT(0), GPL(0), DTL(0),
		st0(0), st1(0), st2(0), st3(0), status(0)
		{
			INITARRAY( SeekSta, SK_STOP );
			INITARRAY( NCN, 0 );
			INITARRAY( PCN, 0 );
		}
};

// ミニフロッピーディスク 各種情報
struct DISK60 {
	int DAC;			// Data Accepted	:データ受信完
	int RFD;			// Ready For Data	:データ受信準備完
	int DAV;			// Data Valid		:データ送信完
	
	int command;		// 受け取ったコマンド
	int step;			// パラメータ入力待ちステータス
	
	int blk;			// 転送ブロック数
	int drv;			// ドライブ番号-1
	int trk;			// トラック番号
	int sct;			// セクタ番号
	
	int rsize;			// 読込みバッファのデータ数
	int wsize;			// 書込みバッファのデータ数
	int ridx;
	
	int size;			// 処理するバイト数
	
	BYTE retdat;		// port D0H から返す値
	
	BYTE busy;			// ドライブBUSY 1:ドライブ1 2:ドライブ2
	
	DISK60() :
		DAC(0), RFD(0), DAV(0),
		command(IDLE), step(0),
		blk(0), drv(0), trk(0), sct(0),
		size(0),
		retdat(0xff), busy(0) {}
};

////////////////////////////////////////////////////////////////
// クラス定義
////////////////////////////////////////////////////////////////
class DSK6 { /// : public P6DEVICE, public IDoko {
protected:
	int DrvNum;							// ドライブ数
	char FilePath[MAXDRV][PATH_MAX];	// ファイルパス
	cD88 *Dimg[MAXDRV];					// ディスクイメージオブジェクトへのポインタ
	bool Sys[MAXDRV];					// システムディスクフラグ
	int waitcnt;						// ウェイトカウンタ
	
	void ResetWait();					// ウェイトカウンタリセット
	void AddWait( int );				// ウェイトカウンタ加算
	bool SetWait( int );				// ウェイト設定
	
public:
	DSK6(); /// VM6 *, const P6ID& );			// コンストラクタ
	virtual ~DSK6();					// デストラクタ
	
	virtual void EventCallback( int, int );	// イベントコールバック関数
	
	virtual bool Init( int ) = 0;		// 初期化
	virtual void Reset() = 0;			// リセット
	
	bool Mount( int, char * );			// DISK マウント
	void Unmount( int );				// DISK アンマウント
	
	int GetDrives();					// ドライブ数取得
	
	bool IsMount( int );				// マウント済み?
	bool IsSystem( int );				// システムディスク?
	bool IsProtect( int );				// プロテクト?
	virtual bool InAccess( int ) = 0;	// アクセス中?
	
	const char *GetFile( int );			// ファイルパス取得
	const char *GetName( int );			// DISK名取得
};

class DSK60 : public DSK6 { /// , public Device {
private:
	DISK60 mdisk;			// ミニフロッピーディスク各種情報
	
	BYTE RBuf[4096];		// 読込みバッファ
	BYTE WBuf[4096];		// 書込みバッファ
	
	BYTE io_D1H;
	BYTE io_D2H;
	
	BYTE FddIn();			// DISKユニットからのデータ入力 		(port D0H)
	void FddOut( BYTE );	// DISKユニットへのコマンド，データ出力 (port D1H)
	BYTE FddCntIn();		// DISKユニットからの制御信号入力 		(port D2H)
	void FddCntOut( BYTE );	// DISKユニットへの制御信号出力 		(port D3H)	
public:
	// I/Oアクセス関数
	void OutD1H( int, BYTE );
	void OutD2H( int, BYTE );
	void OutD3H( int, BYTE );
	BYTE InD0H( int );
	BYTE InD1H( int );
	BYTE InD2H( int );
	DSK60();			// コンストラクタ
	~DSK60();							// デストラクタ
	
	void EventCallback( int, int );		// イベントコールバック関数
	
	bool Init( int );					// 初期化
	void Reset();						// リセット
	bool InAccess( int );				// アクセス中?
	
	// デバイスID
	enum IDOut{ outD1H=0, outD2H, outD3H };
	enum IDIn {  inD0H=0,  inD1H,  inD2H };
};

class SYSTEM : public DEVICE
{
private:
	DEVICE *d_pio, *d_fdc;
public:
	SYSTEM(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~SYSTEM() {}
	
	// common functions
	void initialize();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void open_disk(int drv, _TCHAR* file_path, int offset);
	void close_disk(int drv);

	// unique functions
	void set_context_pio(DEVICE* device) {
		d_pio = device;
	}
	void set_context_fdc(DEVICE* device) {
		d_fdc = device;
	}
};

#endif

