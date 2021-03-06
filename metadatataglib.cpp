/*************************************************************************

  Copyright 2011 Ibrahim Sha'ath

  This file is part of KeyFinder.

  KeyFinder is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  KeyFinder is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with KeyFinder.  If not, see <http://www.gnu.org/licenses/>.

*************************************************************************/

#include "metadatataglib.h"

/*
  Note that a lot remains untested here. For example, I didn't have any ASF or
  APE files to hand while writing it, and I wouldn't be surprised if there are
  many edge cases, though they should at least fail gracefully.
*/

QMutex constructor_mutex; // global mutex on file resolution

TagLibMetadata::TagLibMetadata(const QString& filePath){

  QMutexLocker locker(&constructor_mutex); // mutex the constructor

  QString fileExt = filePath.mid(filePath.lastIndexOf(".") + 1).toLower();

#ifdef Q_OS_WIN
  const wchar_t* filePathCh = reinterpret_cast<const wchar_t *>(filePath.constData());
#else
  QByteArray encodedPath = QFile::encodeName(filePath);
  const char* filePathCh = encodedPath;
#endif

  f = NULL;

  if(fileExt == "mp3")
    f = new TagLib::MPEG::File(filePathCh);
  if(fileExt == "aif" || fileExt == "aiff")
    f = new TagLib::RIFF::AIFF::File(filePathCh);
  if(fileExt == "wav")
    f = new TagLib::RIFF::WAV::File(filePathCh);
  if(fileExt == "ogg")
    f = new TagLib::Ogg::Vorbis::File(filePathCh);
  if(fileExt == "oga"){
    f = new TagLib::Ogg::FLAC::File(filePathCh);
    if (f == NULL || !f->isValid()){
      delete f;
      f = new TagLib::Ogg::Vorbis::File(filePathCh);
    }
  }
  if(fileExt == "flac")
    f = new TagLib::FLAC::File(filePathCh);
  //  if(fileExt == "ape") f = new TagLib::APE::File(filePathCh);
  if(fileExt == "mpc")
    f = new TagLib::MPC::File(filePathCh);
  if(fileExt == "wv")
    f = new TagLib::WavPack::File(filePathCh);
  if(fileExt == "spx")
    f = new TagLib::Ogg::Speex::File(filePathCh);
  if(fileExt == "tta")
    f = new TagLib::TrueAudio::File(filePathCh);
#ifdef TAGLIB_WITH_MP4
  if(fileExt == "m4a" || fileExt == "m4b" || fileExt == "m4p" || fileExt == "mp4" || fileExt == "3g2")
    f = new TagLib::MP4::File(filePathCh);
#endif
#ifdef TAGLIB_WITH_ASF
  if(fileExt == "wma" || fileExt == "asf")
    f = new TagLib::ASF::File(filePathCh);
#endif

  locker.~QMutexLocker(); // unlock mutex

  if(f != NULL && f->isValid())
    return; // everything's fine.

  // or else...
  f = NULL;
#ifdef Q_OS_WIN
  qDebug("TagLib returned NULL File for %s", utf16_to_utf8(filePathCh));
#else
  qDebug("TagLib returned NULL File for %s", filePathCh);
#endif
  return;
}

TagLibMetadata::~TagLibMetadata(){
  if(f != NULL)
    delete f;
}

QString TagLibMetadata::getTitle() const{
  if(f == NULL || !f->isValid())
    return GuiStrings::getInstance()->notApplicable();
  TagLib::String out = f->tag()->title();
  return QString::fromUtf8(out.toCString(true));
}

QString TagLibMetadata::getArtist() const{
  if(f == NULL || !f->isValid())
    return GuiStrings::getInstance()->notApplicable();
  TagLib::String out = f->tag()->artist();
  return QString::fromUtf8(out.toCString(true));
}

QString TagLibMetadata::getComment() const{
  if(f == NULL || !f->isValid())
    return GuiStrings::getInstance()->notApplicable();
  TagLib::FLAC::File* fileTestFlac = dynamic_cast<TagLib::FLAC::File*>(f);
  if(fileTestFlac != NULL){
    // TagLib's default behaviour treats Description as Comment: override
    if(fileTestFlac->xiphComment()->contains("COMMENT")){
      TagLib::String out = fileTestFlac->xiphComment()->fieldListMap()["COMMENT"].toString();
      return QString::fromUtf8((out.toCString()));
    }else{
      return "";
    }
  }else{
    TagLib::String out = f->tag()->comment();
    return QString::fromUtf8(out.toCString(true));
  }
}

QString TagLibMetadata::getGrouping() const{

  if(f == NULL || !f->isValid())
    return GuiStrings::getInstance()->notApplicable();

  TagLib::MPEG::File* fileTestMpeg = dynamic_cast<TagLib::MPEG::File*>(f);
  if(fileTestMpeg != NULL){
    TagLib::ID3v2::Tag* tagTestId3v2 = fileTestMpeg->ID3v2Tag();
    if(tagTestId3v2 != NULL){
      TagLib::ID3v2::FrameList l = tagTestId3v2->frameListMap()["TIT1"];
      if(!l.isEmpty()){
        TagLib::String out = l.front()->toString();
        return QString::fromUtf8((out.toCString()));
      }
      return "";
    }
    TagLib::ID3v1::Tag* tagTestId3v1 = fileTestMpeg->ID3v1Tag();
    if(tagTestId3v1 != NULL){
      return GuiStrings::getInstance()->notApplicable();
    }
  }

  TagLib::RIFF::AIFF::File* fileTestAiff = dynamic_cast<TagLib::RIFF::AIFF::File*>(f);
  if(fileTestAiff != NULL){
    TagLib::ID3v2::Tag* tagTestId3v2 = fileTestAiff->tag();
    if(tagTestId3v2 != NULL){
      TagLib::ID3v2::FrameList l = tagTestId3v2->frameListMap()["TIT1"];
      if(!l.isEmpty()){
        TagLib::String out = l.front()->toString();
        return QString::fromUtf8((out.toCString()));
      }
      return "";
    }
  }

  TagLib::RIFF::WAV::File* fileTestWav = dynamic_cast<TagLib::RIFF::WAV::File*>(f);
  if(fileTestWav != NULL){
    TagLib::ID3v2::Tag* tagTestId3v2 = fileTestWav->tag();
    if(tagTestId3v2 != NULL){
      TagLib::ID3v2::FrameList l = tagTestId3v2->frameListMap()["TIT1"];
      if(!l.isEmpty()){
        TagLib::String out = l.front()->toString();
        return QString::fromUtf8((out.toCString()));
      }
      return "";
    }
  }

  TagLib::MP4::Tag* tagTestMp4 = dynamic_cast<TagLib::MP4::Tag*>(f->tag());
  if(tagTestMp4 != NULL){
    TagLib::MP4::Item m = tagTestMp4->itemListMap()["\251grp"];
    if(m.isValid()){
      TagLib::String out = m.toStringList().front();
      return QString::fromUtf8((out.toCString()));
    }
    return "";
  }

  TagLib::ASF::Tag* tagTestAsf = dynamic_cast<TagLib::ASF::Tag*>(f->tag());
  if(tagTestAsf != NULL){
    TagLib::ASF::AttributeList l = tagTestAsf->attributeListMap()["WM/ContentGroupDescription"];
    if(!l.isEmpty()){
      TagLib::String out = l.front().toString();
      return QString::fromUtf8((out.toCString()));
    }
    return "";
  }

  TagLib::APE::Tag* tagTestApe = dynamic_cast<TagLib::APE::Tag*>(f->tag());
  if(tagTestApe != NULL){
    TagLib::APE::Item m = tagTestApe->itemListMap()["Grouping"];
    if(!m.isEmpty()){
      TagLib::String out = m.toStringList().front();
      return QString::fromUtf8((out.toCString()));
    }
    return "";
  }

#ifdef Q_OS_WIN
  qDebug("Grouping tag read failed all tests on %s",utf16_to_utf8(f->name()));
#else
  qDebug("Grouping tag read failed all tests on %s",f->name());
#endif
  return GuiStrings::getInstance()->notApplicable();
}

QString TagLibMetadata::getKey() const{

  if(f == NULL || !f->isValid())
    return GuiStrings::getInstance()->notApplicable();

  TagLib::MPEG::File* fileTestMpeg = dynamic_cast<TagLib::MPEG::File*>(f);
  if(fileTestMpeg != NULL){
    TagLib::ID3v2::Tag* tagTestId3v2 = fileTestMpeg->ID3v2Tag();
    if(tagTestId3v2 != NULL){
      TagLib::ID3v2::FrameList l = tagTestId3v2->frameListMap()["TKEY"];
      if(!l.isEmpty()){
        TagLib::String out = l.front()->toString();
        return QString::fromUtf8((out.toCString()));
      }
      return "";
    }
    TagLib::ID3v1::Tag* tagTestId3v1 = fileTestMpeg->ID3v1Tag();
    if(tagTestId3v1 != NULL){
      return GuiStrings::getInstance()->notApplicable();
    }
  }

  TagLib::RIFF::AIFF::File* fileTestAiff = dynamic_cast<TagLib::RIFF::AIFF::File*>(f);
  if(fileTestAiff != NULL){
    TagLib::ID3v2::Tag* tagTestId3v2 = fileTestAiff->tag();
    if(tagTestId3v2 != NULL){
      TagLib::ID3v2::FrameList l = tagTestId3v2->frameListMap()["TKEY"];
      if(!l.isEmpty()){
        TagLib::String out = l.front()->toString();
        return QString::fromUtf8((out.toCString()));
      }
      return "";
    }
  }

  TagLib::RIFF::WAV::File* fileTestWav = dynamic_cast<TagLib::RIFF::WAV::File*>(f);
  if(fileTestWav != NULL){
    TagLib::ID3v2::Tag* tagTestId3v2 = fileTestWav->tag();
    if(tagTestId3v2 != NULL){
      TagLib::ID3v2::FrameList l = tagTestId3v2->frameListMap()["TKEY"];
      if(!l.isEmpty()){
        TagLib::String out = l.front()->toString();
        return QString::fromUtf8((out.toCString()));
      }
      return "";
    }
  }

  TagLib::MP4::Tag* tagTestMp4 = dynamic_cast<TagLib::MP4::Tag*>(f->tag());
  if(tagTestMp4 != NULL){
    TagLib::MP4::Item m = tagTestMp4->itemListMap()["----:com.apple.iTunes:initialkey"];
    if(m.isValid()){
      TagLib::String out = m.toStringList().front();
      return QString::fromUtf8((out.toCString()));
    }
    return "";
  }

  TagLib::ASF::Tag* tagTestAsf = dynamic_cast<TagLib::ASF::Tag*>(f->tag());
  if(tagTestAsf != NULL){
    TagLib::ASF::AttributeList l = tagTestAsf->attributeListMap()["WM/InitialKey"];
    if(!l.isEmpty()){
      TagLib::String out = l.front().toString();
      return QString::fromUtf8((out.toCString()));
    }
    return "";
  }

  TagLib::APE::Tag* tagTestApe = dynamic_cast<TagLib::APE::Tag*>(f->tag());
  if(tagTestApe != NULL){
    return GuiStrings::getInstance()->notApplicable();
  }

#ifdef Q_OS_WIN
  qDebug("Key tag read failed all tests on %s",utf16_to_utf8(f->name()));
#else
  qDebug("Key tag read failed all tests on %s",f->name());
#endif
  return GuiStrings::getInstance()->notApplicable();
}

QString TagLibMetadata::writeKeyToMetadata(int key, const Preferences& prefs){

  QString result = "";

  QString dataToWrite = prefs.getKeyCode(key);
  QString delim = prefs.getMetadataDelimiter();

  if(prefs.getMetadataWriteComment() == METADATA_WRITE_OVERWRITE){
    if(getComment() != dataToWrite && setComment(dataToWrite.toLocal8Bit().constData()) == 0)
      result += "c";
  }else if(prefs.getMetadataWriteComment() == METADATA_WRITE_PREPEND){
    if(getComment().left(dataToWrite.length()) != dataToWrite && setComment(dataToWrite.toLocal8Bit().constData() + delim + getComment()) == 0)
      result += "c";
  }else if(prefs.getMetadataWriteComment() == METADATA_WRITE_APPEND){
    if(getComment().right(dataToWrite.length()) != dataToWrite && setComment(getComment() + delim + dataToWrite.toLocal8Bit().constData()) == 0)
      result += "c";
  }

  if(prefs.getMetadataWriteGrouping() == METADATA_WRITE_OVERWRITE){
    if(getGrouping() != dataToWrite && setGrouping(dataToWrite.toLocal8Bit().constData()) == 0)
      result += "g";
  }else if(prefs.getMetadataWriteGrouping() == METADATA_WRITE_PREPEND){
    if(getGrouping().left(dataToWrite.length()) != dataToWrite && setGrouping(dataToWrite.toLocal8Bit().constData() + delim + getGrouping()) == 0)
      result += "g";
  }else if(prefs.getMetadataWriteGrouping() == METADATA_WRITE_APPEND){
    if(getGrouping().right(dataToWrite.length()) != dataToWrite && setGrouping(getGrouping() + delim + dataToWrite.toLocal8Bit().constData()) == 0)
      result += "g";
  }

  dataToWrite = dataToWrite.left(3); // Key field in ID3 holds only 3 chars
  if(prefs.getMetadataWriteKey() == METADATA_WRITE_OVERWRITE)
    if(getKey() != dataToWrite && setKey(dataToWrite.toLocal8Bit().constData()) == 0)
        result += "k";

  return result;

}

int TagLibMetadata::setComment(const QString& cmt){
  if(f == NULL || !f->isValid()){
    qDebug("Cannot set Comment tag on invalid file object");
    return 1;
  }

  // TagLib's default behaviour for FLACs treats Description as Comment. Override.
  TagLib::FLAC::File* fileTestFlac = dynamic_cast<TagLib::FLAC::File*>(f);
  if(fileTestFlac != NULL){
    fileTestFlac->xiphComment()->addField("COMMENT",TagLib::String(cmt.toLocal8Bit().constData()),true);
    f->save();
    return 0;
  }

  // non-FLAC behaviour
  f->tag()->setComment(TagLib::String(cmt.toLocal8Bit().constData()));
  f->save();

  // iTunes hack in addition... is it worth doing this for other id3v2 formats?
  TagLib::MPEG::File* fileTestMpeg = dynamic_cast<TagLib::MPEG::File*>(f);
  if(fileTestMpeg != NULL){
    TagLib::ID3v2::Tag* tagTestId3v2 = fileTestMpeg->ID3v2Tag();
    if(tagTestId3v2 != NULL){
      const TagLib::ID3v2::FrameList &comments = tagTestId3v2->frameListMap()["COMM"];
      bool found = false;
      for(TagLib::ID3v2::FrameList::ConstIterator it = comments.begin(); it != comments.end(); it++){
        // overwrite all appropriate comment elements
        TagLib::ID3v2::CommentsFrame *commFrame = dynamic_cast<TagLib::ID3v2::CommentsFrame *>(*it);
        if(commFrame && commFrame->description().isEmpty()){
          commFrame->setLanguage("eng"); // this is the key.
          commFrame->setText(TagLib::String(cmt.toLocal8Bit().constData()));
          f->save();
          found = true;
        }
      }
      if(found) return 0;
      // didn't find it
      TagLib::ID3v2::CommentsFrame* frm = new TagLib::ID3v2::CommentsFrame();
      frm->setText(TagLib::String(cmt.toLocal8Bit().constData()));
      frm->setLanguage("eng");
      tagTestId3v2->addFrame(frm);
      f->save();
      return 0;
    }
  }

  return 0;
}

int TagLibMetadata::setGrouping(const QString& grp){

  if(f == NULL || !f->isValid()){
    qDebug("Cannot set Grouping tag on invalid file object");
    return 1;
  }

  TagLib::MPEG::File* fileTestMpeg = dynamic_cast<TagLib::MPEG::File*>(f);
  if(fileTestMpeg != NULL){
    TagLib::ID3v2::Tag* tagTestId3v2 = fileTestMpeg->ID3v2Tag();
    if(tagTestId3v2 != NULL){
      TagLib::ID3v2::Frame* frm = new TagLib::ID3v2::TextIdentificationFrame("TIT1");
      frm->setText(TagLib::String(grp.toLocal8Bit().constData()));
      tagTestId3v2->removeFrames("TIT1");
      tagTestId3v2->addFrame(frm);
      f->save();
      return 0;
    }else{
      TagLib::ID3v1::Tag* tagTestId3v1 = fileTestMpeg->ID3v1Tag();
      if(tagTestId3v1 != NULL){
#ifdef Q_OS_WIN
        qDebug("ID3v1 does not support the Grouping tag (%s)",utf16_to_utf8(f->name()));
#else
        qDebug("ID3v1 does not support the Grouping tag (%s)",f->name());
#endif
        return 1;
      }
    }
  }

  TagLib::RIFF::AIFF::File* fileTestAiff = dynamic_cast<TagLib::RIFF::AIFF::File*>(f);
  if(fileTestAiff != NULL){
    TagLib::ID3v2::Tag* tagTestId3v2 = fileTestAiff->tag();
    if(tagTestId3v2 != NULL){
      TagLib::ID3v2::Frame* frm = new TagLib::ID3v2::TextIdentificationFrame("TIT1");
      frm->setText(TagLib::String(grp.toLocal8Bit().constData()));
      tagTestId3v2->removeFrames("TIT1");
      tagTestId3v2->addFrame(frm);
      f->save();
      return 0;
    }
  }

  TagLib::RIFF::WAV::File* fileTestWav = dynamic_cast<TagLib::RIFF::WAV::File*>(f);
  if(fileTestWav != NULL){
    TagLib::ID3v2::Tag* tagTestId3v2 = fileTestWav->tag();
    if(tagTestId3v2 != NULL){
      TagLib::ID3v2::Frame* frm = new TagLib::ID3v2::TextIdentificationFrame("TIT1");
      frm->setText(TagLib::String(grp.toLocal8Bit().constData()));
      tagTestId3v2->removeFrames("TIT1");
      tagTestId3v2->addFrame(frm);
      f->save();
      return 0;
    }
  }

  TagLib::MP4::Tag* tagTestMp4 = dynamic_cast<TagLib::MP4::Tag*>(f->tag());
  if(tagTestMp4 != NULL){
    TagLib::StringList sl(TagLib::String(grp.toLocal8Bit().constData()));
    tagTestMp4->itemListMap()["\251grp"] = sl;
    tagTestMp4->save();
    f->save();
    return 0;
  }

  TagLib::ASF::Tag* tagTestAsf = dynamic_cast<TagLib::ASF::Tag*>(f->tag());
  if(tagTestAsf != NULL){
    tagTestAsf->setAttribute("WM/ContentGroupDescription",TagLib::String(grp.toLocal8Bit().constData()));
    f->save();
    return 0;
  }

  TagLib::APE::Tag* tagTestApe = dynamic_cast<TagLib::APE::Tag*>(f->tag());
  if(tagTestApe != NULL){
    tagTestApe->addValue("GROUPING",TagLib::String(grp.toLocal8Bit().constData()));
    f->save();
    return 0;
  }

#ifdef Q_OS_WIN
  qDebug("Grouping tag write failed all tests on %s",utf16_to_utf8(f->name()));
#else
  qDebug("Grouping tag write failed all tests on %s",f->name());
#endif
  return 1;
}

int TagLibMetadata::setKey(const QString& key){

  if(f == NULL || !f->isValid()){
    qDebug("Cannot set Key tag on invalid file object");
    return 1;
  }

  TagLib::MPEG::File* fileTestMpeg = dynamic_cast<TagLib::MPEG::File*>(f);
  if(fileTestMpeg != NULL){
    TagLib::ID3v2::Tag* tagTestId3v2 = fileTestMpeg->ID3v2Tag();
    if(tagTestId3v2 != NULL){
      TagLib::ID3v2::Frame* frm = new TagLib::ID3v2::TextIdentificationFrame("TKEY");
      frm->setText(TagLib::String(key.toLocal8Bit().constData()));
      tagTestId3v2->removeFrames("TKEY");
      tagTestId3v2->addFrame(frm);
      f->save();
      return 0;
    }else{
      TagLib::ID3v1::Tag* tagTestId3v1 = fileTestMpeg->ID3v1Tag();
      if(tagTestId3v1 != NULL){
#ifdef Q_OS_WIN
        qDebug("ID3v1 does not support the Key tag (%s)",utf16_to_utf8(f->name()));
#else
        qDebug("ID3v1 does not support the Key tag (%s)",f->name());
#endif
        return 1;
      }
    }
  }

  TagLib::RIFF::AIFF::File* fileTestAiff = dynamic_cast<TagLib::RIFF::AIFF::File*>(f);
  if(fileTestAiff != NULL){
    TagLib::ID3v2::Tag* tagTestId3v2 = fileTestAiff->tag();
    if(tagTestId3v2 != NULL){
      TagLib::ID3v2::Frame* frm = new TagLib::ID3v2::TextIdentificationFrame("TKEY");
      frm->setText(TagLib::String(key.toLocal8Bit().constData()));
      tagTestId3v2->removeFrames("TKEY");
      tagTestId3v2->addFrame(frm);
      f->save();
      return 0;
    }
  }

  TagLib::RIFF::WAV::File* fileTestWav = dynamic_cast<TagLib::RIFF::WAV::File*>(f);
  if(fileTestWav != NULL){
    TagLib::ID3v2::Tag* tagTestId3v2 = fileTestWav->tag();
    if(tagTestId3v2 != NULL){
      TagLib::ID3v2::Frame* frm = new TagLib::ID3v2::TextIdentificationFrame("TKEY");
      frm->setText(TagLib::String(key.toLocal8Bit().constData()));
      tagTestId3v2->removeFrames("TKEY");
      tagTestId3v2->addFrame(frm);
      f->save();
      return 0;
    }
  }

  TagLib::MP4::Tag* tagTestMp4 = dynamic_cast<TagLib::MP4::Tag*>(f->tag());
  if(tagTestMp4 != NULL){
    TagLib::StringList sl(TagLib::String(key.toLocal8Bit().constData()));
    tagTestMp4->itemListMap()["----:com.apple.iTunes:initialkey"] = sl;
    tagTestMp4->save();
    f->save();
    return 0;
  }

  TagLib::ASF::Tag* tagTestAsf = dynamic_cast<TagLib::ASF::Tag*>(f->tag());
  if(tagTestAsf != NULL){
    tagTestAsf->setAttribute("WM/InitialKey",TagLib::String(key.toLocal8Bit().constData()));
    f->save();
    return 0;
  }

  TagLib::APE::Tag* tagTestApe = dynamic_cast<TagLib::APE::Tag*>(f->tag());
  if(tagTestApe != NULL){
#ifdef Q_OS_WIN
    qDebug("APE metadata does not support the Key tag (%s)",utf16_to_utf8(f->name()));
#else
    qDebug("APE metadata does not support the Key tag (%s)",f->name());
#endif
    return 1;
  }

#ifdef Q_OS_WIN
  qDebug("Key tag write failed all tests on %s",utf16_to_utf8(f->name()));
#else
  qDebug("Key tag write failed all tests on %s",f->name());
#endif
  return 1;
}
