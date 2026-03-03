# TVTest Android 環境構築進捗

## ✅ 完了済み

### 1. JDK 17
- [x] インストール済み: `C:\Program Files\Java\jdk-17.0.18+8`
- [x] JAVA_HOME設定済み
- [x] PATH設定済み

**確認コマンド**:
```powershell
java -version
# 期待値: openjdk version "17.0.18" 2026-01-20

$env:JAVA_HOME  
# 期待値: C:\Program Files\Java\jdk-17.0.18+8
```

### 2. Android Studio
- [x] インストール済み
- [x] SDK Manager設定済み
- [x] AVD作成済み: TVTest_Pixel7Pro_API33

**確認方法**:
```
1. Android Studio起動
2. Tools → SDK Manager
   - Android 13.0 (API 33) がインストール済み
   - NDK (Side by side) 25.2.9519653 がインストール済み
3. Tools → AVD Manager  
   - TVTest_Pixel7Pro_API33 が存在
```

## 🔄 次のステップ: Qt Creator

### 3. Qt 6.6.0 + Android
- [ ] Qt Online Installer実行
- [ ] コンポーネント選択:
  ```
  Qt 6.6.0:
  ├── ☑ MinGW 11.2.0 64-bit
  ├── ☑ Android ARM64-v8a  
  ├── ☑ Android ARMv7
  ├── ☑ Android x86_64
  ├── ☑ Qt Multimedia
  └── ☑ Qt SQL
  
  Tools:
  ├── ☑ Qt Creator 12.x
  ├── ☑ Qt Designer
  └── ☑ CMake
  ```

### 4. Qt Creator Android設定
```
Tools → Options → Devices → Android:

JDK Location: C:\Program Files\Java\jdk-17.0.18+8

Android SDK: C:\Users\[User]\AppData\Local\Android\Sdk

Android NDK: C:\Users\[User]\AppData\Local\Android\Sdk\ndk\25.2.9519653
```

## 🧪 最終確認テスト

再起動後に実行:

### 1. 環境変数確認
```powershell
# PowerShell
java -version
javac -version  
$env:JAVA_HOME
$env:PATH | Select-String "Java"
```

### 2. Android Studio確認
```
1. スタートメニュー → Android Studio
2. Welcome画面が表示される
3. Tools → AVD Manager → エミュレータ起動テスト
```

### 3. Qt Creator確認
```
1. スタートメニュー → Qt Creator  
2. Tools → Options → Devices → Android
3. Test ボタンで全項目 ✅
```

### 4. エミュレータ起動テスト
```
# Android Studio AVD Manager
1. TVTest_Pixel7Pro_API33 の ▷ (Play)クリック
2. エミュレータが起動してAndroidホーム画面表示
3. adb devices で認識確認
```

## トラブルシューティング

### Java関連
- JAVA_HOME未設定 → システム環境変数で再設定
- PATHが通らない → システム再起動

### Android Studio関連  
- AVD起動エラー → Intel HAXM再インストール
- SDK認識エラー → SDK Manager再設定

### Qt Creator関連
- Android設定エラー → パス設定見直し
- NDK未認識 → NDKバージョン確認

---
**次回作業**: Qt Creator Android設定完了後、TVTestプロジェクト作成開始