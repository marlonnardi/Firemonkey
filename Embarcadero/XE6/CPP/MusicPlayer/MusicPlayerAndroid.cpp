// ---------------------------------------------------------------------------

#pragma hdrstop

#include "MusicPlayerAndroid.h"
// ---------------------------------------------------------------------------
#ifdef __ANDROID__
using namespace Musicplayer::Utils;

namespace Musicplayer {
	namespace Android {
		Fmx::Graphics::TBitmap * NoArtBitmap;
		TMusicPlayer* TMusicPlayer::FInstance;

		inline __fastcall TMusicPlayer::TMusicPlayer
			(Utils::TMPControllerType AType) {
			FMusicPlayer = TJMediaPlayer::Create();
			FPlayBackState = Utils::TMPPlaybackState::Stopped;
			FRepeatMode = Utils::TMPRepeatMode::All;
			FShuffleMode = false;
			FDefaultAlbumImage =
				new TBitmap(TPath::Combine(TPath::GetDocumentsPath(),
				"MusicNote.png"));
			(new TProcessThread(true, this, DoOnProcessPlay))->Start();
		}

		__fastcall TMusicPlayer::~TMusicPlayer(void) {
			FMusicPlayer->release();
		}

		void __fastcall TMusicPlayer::DoOnSongChange(int newIndex) {
			if (FOnSongChange) {
				TThread::Queue(TThread::CurrentThread,
					new MyThreadMethod<TOnSongChangeEvent, int>(FOnSongChange,
					newIndex));
			}
		}

		void __fastcall TMusicPlayer::DoOnProcessPlay(float newPos) {
			if (FOnProcessPlay) {
				TThread::Queue(TThread::CurrentThread,
					new MyThreadMethod<TOnProcessPlayEvent, float>
					(FOnProcessPlay, newPos));
			}
		}

		void __fastcall TMusicPlayer::SetVolume(const float Value) {
			FMusicPlayer->setVolume(Value, Value);
		}

		void __fastcall TMusicPlayer::SetTime(const float Value) {
			FMusicPlayer->seekTo((int) Value);
		}

		void __fastcall TMusicPlayer::SetRepeatMode
			(const Utils::TMPRepeatMode Value) {
			FRepeatMode = Value;
		}

		void __fastcall TMusicPlayer::SetShuffleMode(const bool Value) {
			FShuffleMode = Value;
		}

		float __fastcall TMusicPlayer::GetVolume(void) {
			int result =
				((_di_JAudioManager)Fmx::Platform::Android::MainActivity()
				->getSystemService(TJContext::JavaClass->AUDIO_SERVICE))
				->getStreamVolume
				(Androidapi::Jni::Media::TJAudioManager::JavaClass->
				STREAM_MUSIC);
			return result /
				((_di_JAudioManager)Fmx::Platform::Android::MainActivity()
				->getSystemService(TJContext::JavaClass->AUDIO_SERVICE))
				->getStreamMaxVolume
				(Androidapi::Jni::Media::TJAudioManager::JavaClass->
				STREAM_MUSIC);
		}

		float __fastcall TMusicPlayer::GetTime(void) {
			return FMusicPlayer->getCurrentPosition();
		}

		Utils::TMPRepeatMode __fastcall TMusicPlayer::GetRepeatMode(void) {
			return FRepeatMode;
		}

		float __fastcall TMusicPlayer::GetDuration(void) {
			return FMusicPlayer->getDuration();
		}

		Utils::TMPPlaybackState __fastcall TMusicPlayer::GetPlaybackState(void)
		{
			return FPlayBackState;
		}

		bool __fastcall TMusicPlayer::GetShuffleMode(void) {
			return FShuffleMode;
		}

		void __fastcall TMusicPlayer::SetPlayerType
			(Utils::TMPControllerType AType) {
			// Do nothing
		}

		TMusicPlayer* __fastcall TMusicPlayer::DefaultPlayer() {
			if (!FInstance) {
				FInstance = new TMusicPlayer(TMPControllerType::App);
			}
			return FInstance;
		}

		System::DynamicArray<System::UnicodeString>__fastcall
			TMusicPlayer::GetAlbums(void) {
			TJavaObjectArray__1<_di_JString> *projection;
			_di_JCursor cursor;
			_di_Jnet_Uri art_uri, uri;
			System::DynamicArray<System::UnicodeString>result;

			projection = new TJavaObjectArray__1<_di_JString>(3);
			projection->Items[0] = TJAudio_AlbumColumns::JavaClass->ALBUM;
			projection->Items[1] = TJAudio_AlbumColumns::JavaClass->ARTIST;
			projection->Items[2] = StringToJString("_id");

			art_uri = TJnet_Uri::JavaClass->parse
				(StringToJString("content://media/external/audio/albumart"));
			cursor = Fmx::Platform::Android::MainActivity()->getContentResolver
				()->query(TJAudio_Albums::JavaClass->EXTERNAL_CONTENT_URI,
				projection, NULL, NULL, NULL);

			result.set_length(cursor->getCount());
			FAlbums.set_length(cursor->getCount() + 1);
			FAlbums[cursor->getCount()] = Utils::TMPAlbum::AllMusicAlbum();
			while (cursor->moveToNext()) {
				Log::d("Albums: CursorPosition :" +
					IntToStr(cursor->getPosition()) + " from " +
					IntToStr(cursor->getCount()));
				FAlbums[cursor->getPosition()].Name =
					JStringToString(cursor->getString(0));
				FAlbums[cursor->getPosition()].Artist =
					JStringToString(cursor->getString(1));
				FAlbums[cursor->getPosition()].Album_ID = cursor->getInt(2);
				uri = TJContentUris::JavaClass->withAppendedId(art_uri,
					FAlbums[cursor->getPosition()].Album_ID);

				if (TFile::Exists(JStringToString(uri->getPath()))) {
					FAlbums[cursor->getPosition()].Artwork =
						new TBitmap(JStringToString(uri->getPath()));
				}
				else {
					FAlbums[cursor->getPosition()].Artwork = FDefaultAlbumImage;
				}
			}
			cursor->close();

			return result;
		}

		System::DynamicArray<System::UnicodeString>__fastcall
			TMusicPlayer::GetSongs(void) {
			TJavaObjectArray__1<_di_JString> *projection;
			_di_JCursor cursor;
			_di_JString selection;
			System::DynamicArray<System::UnicodeString>result;

			selection =
				StringToJString(JStringToString
				(TJAudio_AudioColumns::JavaClass->IS_MUSIC) + " != 0");

			projection = new TJavaObjectArray__1<_di_JString>(5);
			projection->Items[0] = TJAudio_AudioColumns::JavaClass->ARTIST;
			projection->Items[1] = StringToJString("title");
			projection->Items[2] = StringToJString("_data");
			projection->Items[3] = TJAudio_AudioColumns::JavaClass->ALBUM;
			projection->Items[4] = TJAudio_AudioColumns::JavaClass->DURATION;

			cursor = Fmx::Platform::Android::MainActivity()->getContentResolver
				()->query(TJAudio_Media::JavaClass->EXTERNAL_CONTENT_URI,
				projection, selection, NULL, NULL);

			result.set_length(cursor->getCount());
			FPlaylist.set_length(cursor->getCount());

			while (cursor->moveToNext()) {
				FPlaylist[cursor->getPosition()] =
					Utils::TMPSong::FromCursor(cursor);
				result[cursor->getPosition()] =
					"[" + FPlaylist[cursor->getPosition()].Artist + "]-[" +
					FPlaylist[cursor->getPosition()].Title;
			}
			cursor->close();
			return result;
		}

		System::DynamicArray<System::UnicodeString>__fastcall
			TMusicPlayer::GetSongsInAlbum(System::UnicodeString AName) {
			if (AName == TMPAlbum::AllMusicAlbum().Name) {
				return GetSongs();
			}

			TJavaObjectArray__1<_di_JString> *projection;
			_di_JCursor cursor;
			_di_JString selection;
			System::DynamicArray<System::UnicodeString>result;

			selection =
				StringToJString(JStringToString
				(TJAudio_AudioColumns::JavaClass->IS_MUSIC) + " != 0 and " +
				JStringToString(TJAudio_AudioColumns::JavaClass->ALBUM) +
				" = \"" + AName + "\"");

			projection = new TJavaObjectArray__1<_di_JString>(5);
			projection->Items[0] = TJAudio_AudioColumns::JavaClass->ARTIST;
			projection->Items[1] = StringToJString("title");
			projection->Items[2] = StringToJString("_data");
			projection->Items[3] = TJAudio_AudioColumns::JavaClass->ALBUM;
			projection->Items[4] = TJAudio_AudioColumns::JavaClass->DURATION;

			cursor = Fmx::Platform::Android::MainActivity()->getContentResolver
				()->query(TJAudio_Media::JavaClass->EXTERNAL_CONTENT_URI,
				projection, selection, NULL, NULL);

			result.set_length(cursor->getCount());
			FPlaylist.set_length(cursor->getCount());

			while (cursor->moveToNext()) {
				FPlaylist[cursor->getPosition()] =
					Utils::TMPSong::FromCursor(cursor);
				result[cursor->getPosition()] =
					"[" + FPlaylist[cursor->getPosition()].Artist + "]-[" +
					FPlaylist[cursor->getPosition()].Title;
			}
			cursor->close();
			return result;
		}

		bool TMusicPlayer::IsPlaying() {
			return FMusicPlayer->isPlaying();
		}

		void __fastcall TMusicPlayer::PlaySong(System::UnicodeString path) {
			Stop();
			FMusicPlayer->reset();
			FMusicPlayer->setDataSource(StringToJString(path));
			Play();
		}

		void __fastcall TMusicPlayer::PlayByIndex(unsigned Index) {
			FCurrentIndex = Index;
			PlaySong(FPlaylist[FCurrentIndex].Path);
		}

		void __fastcall TMusicPlayer::Play(void) {
			if (FPlayBackState == Utils::TMPPlaybackState::Stopped)
				FMusicPlayer->prepare();
			FMusicPlayer->start();
			FPlayBackState = Utils::TMPPlaybackState::Playing;
		}

		void __fastcall TMusicPlayer::Stop(void) {
			if (FPlayBackState == Utils::TMPPlaybackState::Playing ||
				FPlayBackState == Utils::TMPPlaybackState::Paused) {
				FMusicPlayer->seekTo(0);
			}
			FPlayBackState = Utils::TMPPlaybackState::Stopped;
			FMusicPlayer->stop();
			while (FMusicPlayer->isPlaying()) {
				Sleep(10);
			}
			DoOnProcessPlay(0);
		}

		void __fastcall TMusicPlayer::Pause(void) {
			FMusicPlayer->pause();
			FPlayBackState = Utils::TMPPlaybackState::Paused;
		}

		void __fastcall TMusicPlayer::Next(void) {
			switch (FRepeatMode) {
			case Utils::TMPRepeatMode::One:
				Time = 0;
				Play();
				break;
			case Utils::TMPRepeatMode::None:
			case Utils::TMPRepeatMode::Default:
				if (FCurrentIndex == FPlaylist.Length - 1) {
					FMusicPlayer->stop();
				}
				else {
					if (FShuffleMode)
						PlayByIndex(Random(FPlaylist.Length));
					else
						PlayByIndex(FCurrentIndex + 1);
				}
			case Utils::TMPRepeatMode::All:
				if (FCurrentIndex == FPlaylist.Length - 1) {
					PlayByIndex(0);
				}
				else {
					if (FShuffleMode)
						PlayByIndex(Random(FPlaylist.Length));
					else
						PlayByIndex(FCurrentIndex + 1);
				}
			default:
				break;
			}
			DoOnSongChange(FCurrentIndex);
		}

		void __fastcall TMusicPlayer::Previous(void) {
			if (FCurrentIndex > 0 && FCurrentIndex < FPlaylist.Length) {
				PlayByIndex(FCurrentIndex - 1);
				DoOnSongChange(FCurrentIndex);
			}
		}

		// ------------ TMusicPlayer::TProcessThread --------------------------------
		__fastcall TMusicPlayer::TProcessThread::TProcessThread
			(bool CreateSuspended, TMusicPlayer* AMusicPlayer,
			Musicplayer::Utils::TOnProcessPlayEvent processHandler)
			: TThread(CreateSuspended) {
			FMusicPlayer = AMusicPlayer;
			FOnProcessPlay = processHandler;
		}

		// ---------------------------------------------------------------------------
		__fastcall TMusicPlayer::TProcessThread::~TProcessThread(void) {
			FMusicPlayer = nullptr;
		}

		// ---------------------------------------------------------------------------
		void __fastcall TMusicPlayer::TProcessThread::Execute(void) {
			while (FMusicPlayer) {
				Log::d("Process Thread: Execute");
				switch (FMusicPlayer->PlaybackState) {
				case TMPPlaybackState::Playing:
					ProcessPlay();
					break;
				case TMPPlaybackState::Stopped:
				case TMPPlaybackState::Paused:
				case TMPPlaybackState::Interrupted:
				case TMPPlaybackState::SeekingForward:
				case TMPPlaybackState::SeekingBackward:
					Sleep(200);
					break;
				}
			}
		}

		// ---------------------------------------------------------------------------
		void __fastcall TMusicPlayer::TProcessThread::ProcessPlay(void) {
			float currentPos = FMusicPlayer->Time;
			if (FOnProcessPlay) {
				Log::d("Process Thread: ProcessPlay: call FOnProcessPlay");
				FOnProcessPlay((currentPos / FMusicPlayer->Duration) * 100);
			}

			if (FMusicPlayer->IsPlaying()) {
				Sleep(200);
			}
			else {
				FMusicPlayer->Next();
			}
		}
		// ---------------------------------------------------------------------------

	} /* namespace Android */
} /* namespace Musicplayer */
#endif
#pragma package(smart_init)
