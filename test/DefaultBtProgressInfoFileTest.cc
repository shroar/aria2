#include "DefaultBtProgressInfoFile.h"
#include "Option.h"
#include "Util.h"
#include "Exception.h"
#include "MockBtContext.h"
#include "MockPeerStorage.h"
#include "MockPieceStorage.h"
#include "BtRuntime.h"
#include "BtRegistry.h"
#include "prefs.h"
#include "SingleFileDownloadContext.h"
#include "Piece.h"
#include "AnnounceTier.h"
#include <fstream>
#include <iomanip>
#include <cppunit/extensions/HelperMacros.h>

using namespace std;

class DefaultBtProgressInfoFileTest:public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(DefaultBtProgressInfoFileTest);
  CPPUNIT_TEST(testSave);
  CPPUNIT_TEST(testSave_nonBt);
  CPPUNIT_TEST(testLoad);
  CPPUNIT_TEST(testLoad_nonBt);
  CPPUNIT_TEST_SUITE_END();
private:
  MockBtContextHandle _btContext;
  MockPieceStorageHandle _pieceStorage;
  SharedHandle<Option> _option;
  SharedHandle<BitfieldMan> _bitfield;
public:
  DefaultBtProgressInfoFileTest():_btContext(0), _pieceStorage(0), _option(0), _bitfield(0) {}

  void setUp() {
    BtRegistry::unregisterAll();

    static unsigned char infoHash[] = {
      0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa,
      0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0xff, 0xff, 0xff, 0xff,
    };

    _option = new Option();
    _option->put(PREF_DIR, ".");

    _btContext = new MockBtContext();
    _btContext->setInfoHash(infoHash);

    _bitfield = new BitfieldMan(1024, 80*1024);

    _pieceStorage = new MockPieceStorage();
    _pieceStorage->setBitfield(_bitfield.get());

    MockPeerStorageHandle peerStorage = new MockPeerStorage();

    BtRuntimeHandle btRuntime = new BtRuntime();

    BtRegistry::registerBtContext(_btContext->getInfoHashAsString(),
				  _btContext);
    BtRegistry::registerPieceStorage(_btContext->getInfoHashAsString(),
				     _pieceStorage);
    BtRegistry::registerPeerStorage(_btContext->getInfoHashAsString(),
				    peerStorage);
    BtRegistry::registerBtRuntime(_btContext->getInfoHashAsString(),
				  btRuntime);
  }

  void tearDown()
  {
    BtRegistry::unregisterAll();
  }

  void testSave();
  void testLoad();
  void testSave_nonBt();
  void testLoad_nonBt();
};

#undef BLOCK_LENGTH
#define BLOCK_LENGTH 256

CPPUNIT_TEST_SUITE_REGISTRATION(DefaultBtProgressInfoFileTest);

void DefaultBtProgressInfoFileTest::testLoad()
{
  try {
  _btContext->setName("load");
  _btContext->setPieceLength(1024);
  _btContext->setTotalLength(81920);
  _btContext->setNumPieces(80);

  DefaultBtProgressInfoFile infoFile(_btContext, _pieceStorage, _option.get());
  CPPUNIT_ASSERT_EQUAL(string("./load.aria2"), infoFile.getFilename());

  infoFile.load();

  // check the contents of objects

  // upload length
  CPPUNIT_ASSERT_EQUAL((int64_t)1024, BT_RUNTIME(_btContext)->getUploadLengthAtStartup());

  // bitfield
  CPPUNIT_ASSERT_EQUAL(string("fffffffffffffffffffe"),
		       Util::toHex(_bitfield->getBitfield(), _bitfield->getBitfieldLength()));

  // the number of in-flight pieces
  CPPUNIT_ASSERT_EQUAL((int32_t)2,
		       _pieceStorage->countInFlightPiece());

  // piece index 1
  Pieces inFlightPieces = _pieceStorage->getInFlightPieces();
  PieceHandle piece1 = inFlightPieces[0];
  CPPUNIT_ASSERT_EQUAL((int32_t)1, piece1->getIndex());
  CPPUNIT_ASSERT_EQUAL((int32_t)1024, piece1->getLength());
  CPPUNIT_ASSERT_EQUAL((int32_t)1, piece1->getBitfieldLength());
  CPPUNIT_ASSERT_EQUAL(string("00"), Util::toHex(piece1->getBitfield(),
						 piece1->getBitfieldLength()));

  // piece index 2
  PieceHandle piece2 = inFlightPieces[1];
  CPPUNIT_ASSERT_EQUAL((int32_t)2, piece2->getIndex());
  CPPUNIT_ASSERT_EQUAL((int32_t)512, piece2->getLength());
  
  } catch(Exception* e) {
    cerr << e->getMsg() << endl;
    delete e;
  }
}

void DefaultBtProgressInfoFileTest::testLoad_nonBt()
{
  BtRegistry::unregisterAll();

  SingleFileDownloadContextHandle dctx =
    new SingleFileDownloadContext(1024, 81920, "load-nonBt");
  
  DefaultBtProgressInfoFile infoFile(dctx, _pieceStorage, _option.get());
  CPPUNIT_ASSERT_EQUAL(string("./load-nonBt.aria2"), infoFile.getFilename());
  infoFile.load();

  // check the contents of objects

  // bitfield
  CPPUNIT_ASSERT_EQUAL(string("fffffffffffffffffffe"),
		       Util::toHex(_bitfield->getBitfield(), _bitfield->getBitfieldLength()));

  // the number of in-flight pieces
  CPPUNIT_ASSERT_EQUAL((int32_t)2,
		       _pieceStorage->countInFlightPiece());

  // piece index 1
  Pieces inFlightPieces = _pieceStorage->getInFlightPieces();
  PieceHandle piece1 = inFlightPieces[0];
  CPPUNIT_ASSERT_EQUAL((int32_t)1, piece1->getIndex());
  CPPUNIT_ASSERT_EQUAL((int32_t)1024, piece1->getLength());
  CPPUNIT_ASSERT_EQUAL((int32_t)1, piece1->getBitfieldLength());
  CPPUNIT_ASSERT_EQUAL(string("00"), Util::toHex(piece1->getBitfield(),
						 piece1->getBitfieldLength()));

  // piece index 2
  PieceHandle piece2 = inFlightPieces[1];
  CPPUNIT_ASSERT_EQUAL((int32_t)2, piece2->getIndex());
  CPPUNIT_ASSERT_EQUAL((int32_t)512, piece2->getLength());

}

void DefaultBtProgressInfoFileTest::testSave_nonBt()
{
  BtRegistry::unregisterAll();

  SingleFileDownloadContextHandle dctx =
    new SingleFileDownloadContext(1024, 81920, "save-temp");

  _bitfield->setAllBit();
  _bitfield->unsetBit(79);
  _pieceStorage->setCompletedLength(80896);

  PieceHandle p1 = new Piece(1, 1024);
  PieceHandle p2 = new Piece(2, 512);
  Pieces inFlightPieces;
  inFlightPieces.push_back(p1);
  inFlightPieces.push_back(p2);
  _pieceStorage->addInFlightPiece(inFlightPieces);
  
  DefaultBtProgressInfoFile infoFile(dctx, _pieceStorage, _option.get());
  CPPUNIT_ASSERT_EQUAL(string("./save-temp.aria2"), infoFile.getFilename());

  infoFile.save();
  
  // read and validate
  ifstream in(infoFile.getFilename().c_str());

  //in.exceptions(ios::failbit);

  unsigned char version[2];
  in.read((char*)version, sizeof(version));
  CPPUNIT_ASSERT_EQUAL(string("0000"), Util::toHex(version, sizeof(version)));

  unsigned char extension[4];
  in.read((char*)extension, sizeof(extension));
  CPPUNIT_ASSERT_EQUAL(string("00000000"), Util::toHex(extension, sizeof(extension)));

  int32_t infoHashLength;
  in.read(reinterpret_cast<char*>(&infoHashLength), sizeof(infoHashLength));
  CPPUNIT_ASSERT_EQUAL((int32_t)0, infoHashLength);

  int32_t pieceLength;
  in.read((char*)&pieceLength, sizeof(pieceLength));
  CPPUNIT_ASSERT_EQUAL((int32_t)1024, pieceLength);

  int64_t totalLength;
  in.read((char*)&totalLength, sizeof(totalLength));
  CPPUNIT_ASSERT_EQUAL((int64_t)81920/* 80*1024 */, totalLength);

  int64_t uploadLength;
  in.read((char*)&uploadLength, sizeof(uploadLength));
  CPPUNIT_ASSERT_EQUAL((int64_t)0, uploadLength);

  int32_t bitfieldLength;
  in.read((char*)&bitfieldLength, sizeof(bitfieldLength));
  CPPUNIT_ASSERT_EQUAL((int32_t)10, bitfieldLength);

  unsigned char bitfieldRead[10];
  in.read((char*)bitfieldRead, sizeof(bitfieldRead));
  CPPUNIT_ASSERT_EQUAL(string("fffffffffffffffffffe"),
		       Util::toHex(bitfieldRead, sizeof(bitfieldRead)));

  int32_t numInFlightPiece;
  in.read((char*)&numInFlightPiece, sizeof(numInFlightPiece));
  CPPUNIT_ASSERT_EQUAL((int32_t)2, numInFlightPiece);

  // piece index 1
  int32_t index1;
  in.read((char*)&index1, sizeof(index1));
  CPPUNIT_ASSERT_EQUAL((int32_t)1, index1);

  int32_t pieceLength1;
  in.read((char*)&pieceLength1, sizeof(pieceLength1));
  CPPUNIT_ASSERT_EQUAL((int32_t)1024, pieceLength1);

  int32_t pieceBitfieldLength1;
  in.read((char*)&pieceBitfieldLength1, sizeof(pieceBitfieldLength1));
  CPPUNIT_ASSERT_EQUAL((int32_t)1, pieceBitfieldLength1);

  unsigned char pieceBitfield1[1];
  in.read((char*)pieceBitfield1, sizeof(pieceBitfield1));
  CPPUNIT_ASSERT_EQUAL(string("00"),
		       Util::toHex(pieceBitfield1, sizeof(pieceBitfield1)));

  // piece index 2
  int32_t index2;
  in.read((char*)&index2, sizeof(index2));
  CPPUNIT_ASSERT_EQUAL((int32_t)2, index2);

  int32_t pieceLength2;
  in.read((char*)&pieceLength2, sizeof(pieceLength2));
  CPPUNIT_ASSERT_EQUAL((int32_t)512, pieceLength2);

}

void DefaultBtProgressInfoFileTest::testSave()
{
  _btContext->setName("save-temp");
  _btContext->setPieceLength(1024);
  _btContext->setTotalLength(81920);
  _bitfield->setAllBit();
  _bitfield->unsetBit(79);
  _pieceStorage->setCompletedLength(80896);
  TransferStat stat;
  stat.setAllTimeUploadLength(1024);
  MockPeerStorageHandle(PEER_STORAGE(_btContext))->setStat(stat);

  PieceHandle p1 = new Piece(1, 1024);
  PieceHandle p2 = new Piece(2, 512);
  Pieces inFlightPieces;
  inFlightPieces.push_back(p1);
  inFlightPieces.push_back(p2);
  _pieceStorage->addInFlightPiece(inFlightPieces);
  
  DefaultBtProgressInfoFile infoFile(_btContext, _pieceStorage, _option.get());
  CPPUNIT_ASSERT_EQUAL(string("./save-temp.aria2"), infoFile.getFilename());

  infoFile.save();

  // read and validate
  ifstream in(infoFile.getFilename().c_str());

  //in.exceptions(ios::failbit);

  unsigned char version[2];
  in.read((char*)version, sizeof(version));
  CPPUNIT_ASSERT_EQUAL(string("0000"), Util::toHex(version, sizeof(version)));

  unsigned char extension[4];
  in.read((char*)extension, sizeof(extension));
  CPPUNIT_ASSERT_EQUAL(string("00000001"), Util::toHex(extension, sizeof(extension)));

  int32_t infoHashLength;
  in.read(reinterpret_cast<char*>(&infoHashLength), sizeof(infoHashLength));
  CPPUNIT_ASSERT_EQUAL((int32_t)20, infoHashLength);

  unsigned char infoHashRead[20];
  in.read((char*)infoHashRead, sizeof(infoHashRead));
  CPPUNIT_ASSERT_EQUAL(string("112233445566778899aabbccddeeff00ffffffff"),
		       Util::toHex(infoHashRead, sizeof(infoHashRead)));

  int32_t pieceLength;
  in.read((char*)&pieceLength, sizeof(pieceLength));
  CPPUNIT_ASSERT_EQUAL((int32_t)1024, pieceLength);

  int64_t totalLength;
  in.read((char*)&totalLength, sizeof(totalLength));
  CPPUNIT_ASSERT_EQUAL((int64_t)81920/* 80*1024 */, totalLength);

  int64_t uploadLength;
  in.read((char*)&uploadLength, sizeof(uploadLength));
  CPPUNIT_ASSERT_EQUAL((int64_t)1024, uploadLength);

  int32_t bitfieldLength;
  in.read((char*)&bitfieldLength, sizeof(bitfieldLength));
  CPPUNIT_ASSERT_EQUAL((int32_t)10, bitfieldLength);

  unsigned char bitfieldRead[10];
  in.read((char*)bitfieldRead, sizeof(bitfieldRead));
  CPPUNIT_ASSERT_EQUAL(string("fffffffffffffffffffe"),
		       Util::toHex(bitfieldRead, sizeof(bitfieldRead)));

  int32_t numInFlightPiece;
  in.read((char*)&numInFlightPiece, sizeof(numInFlightPiece));
  CPPUNIT_ASSERT_EQUAL((int32_t)2, numInFlightPiece);

  // piece index 1
  int32_t index1;
  in.read((char*)&index1, sizeof(index1));
  CPPUNIT_ASSERT_EQUAL((int32_t)1, index1);

  int32_t pieceLength1;
  in.read((char*)&pieceLength1, sizeof(pieceLength1));
  CPPUNIT_ASSERT_EQUAL((int32_t)1024, pieceLength1);

  int32_t pieceBitfieldLength1;
  in.read((char*)&pieceBitfieldLength1, sizeof(pieceBitfieldLength1));
  CPPUNIT_ASSERT_EQUAL((int32_t)1, pieceBitfieldLength1);

  unsigned char pieceBitfield1[1];
  in.read((char*)pieceBitfield1, sizeof(pieceBitfield1));
  CPPUNIT_ASSERT_EQUAL(string("00"),
		       Util::toHex(pieceBitfield1, sizeof(pieceBitfield1)));

  // piece index 2
  int32_t index2;
  in.read((char*)&index2, sizeof(index2));
  CPPUNIT_ASSERT_EQUAL((int32_t)2, index2);

  int32_t pieceLength2;
  in.read((char*)&pieceLength2, sizeof(pieceLength2));
  CPPUNIT_ASSERT_EQUAL((int32_t)512, pieceLength2);


}
