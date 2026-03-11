# Kasane Design - UI/UX 프로토타입 계획서

## 1. 개요

**Kasane**는 일렉기타를 오디오 인터페이스에 연결하고 VST/VST3 플러그인을 호스팅하여 즉시 연주할 수 있는 경량화된 앰프/FX 호스트 프로그램입니다.

DAW 의 무거운 기능들을 제거하고 실시간 모니터링과 플러그인 적용에만 집중한 컴팩트한 디자인을 지향합니다.


## 4. 디자인 시스템

### 4.1 컬러 스키엄 (CSS 변수)

#### Dark Mode (기본)

```css
:root[data-theme="dark"] {
  --background: #0f172a;      /* slate-900 */
  --surface: #1e293b;         /* slate-800 */
  --surface-elevated: #334155;/* slate-700 */
  --primary: #06b6d4;         /* cyan-500 */
  --primary-hover: #0891b2;   /* cyan-600 */
  --accent: #3b82f6;          /* blue-500 */
  --success: #10b981;         /* emerald-500 */
  --warning: #f59e0b;         /* amber-500 */
  --danger: #ef4444;          /* red-500 */
  --text-primary: #f1f5f9;    /* slate-100 */
  --text-secondary: #94a3b8;  /* slime-400 */
  --border: #334155;          /* slate-700 */
}
```

#### Light Mode

```css
:root[data-theme="light"] {
  --background: #f8fafc;      /* slate-50 */
  --surface: #ffffff;
  --surface-elevated: #f1f5f9;/* slate-100 */
  --primary: #0891b2;         /* cyan-600 */
  --primary-hover: #0e7490;   /* cyan-700 */
  --accent: #2563eb;          /* blue-600 */
  --success: #059669;         /* emerald-600 */
  --warning: #d97706;         /* amber-600 */
  --danger: #dc2626;          /* red-600 */
  --text-primary: #1e293b;    /* slate-800 */
  --text-secondary: #64748b;  /* slate-500 */
  --border: #e2e8f0;          /* slate-200 */
}
```

### 4.2 타이포그래피

| 용도 | 폰트 | 크기 | 무게 |
|------|------|------|------|
| 본문 | Inter | 14px-16px | 400 |
| 제목 | Inter | 18px-24px | 600 |
| 숫자/게이지 | JetBrains Mono | 14px-20px | 500 |
| 한글/日本語/中文 | Inter + Noto Sans KR | 14px-16px | 400 |

### 4.3 스펜싱

- Base unit: `4px`
- xs: 4px, sm: 8px, md: 16px, lg: 24px, xl: 32px, 2xl: 48px

### 4.4 컴포넌트 스타일

#### 플러그인 카드 (컴팩트)

- Width: 160px
- Height: 80px
- Border-radius: 8px
- Padding: 12px

#### 게인 슬라이더

- Height: 40px
- Track height: 6px
- Thumb diameter: 20px

## 5. 메인 화면 레이아웃

```
┌─────────────────────────────────────────────────────────────────┐
│ ┌─────┐                              [🌐] [🌙] [⚙️] [🎛️] [🎵]  │
│ │ Logo│  Kasane                          en  ▼  Dark  Settings│
│ └─────┘                                                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │ Input  ─────────────────────────────────────  Output     │  │
│  │ [▒▒▒▒▒▒░░░░] -6dB                        [▒▒▒▒▒▒▒▒░░] -3dB│  │
│  │ Guitar Interface In 1                    Interface Out L │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │ PLUGIN CHAIN                                    [+ Add]  │  │
│  │ ┌────┐┌────┐┌────┐┌────┐┌────┐                         │  │
│  │ │AMP │→│OD  │→│EQ  │→│DL  │→│RV  │  →  →  →  →        │  │
│  │ │[●] │ │[●] │ │[○] │ │[●] │ │[●] │                     │  │
│  │ └────┘└────┘└────┘└────┘└────┘                         │  │
│  │  ↑Drag & Drop reorder (horizontal scroll)                │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │ L [████████░░░░░░░░] -12dB    R [█████████░░░░░░░] -10dB │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

## 6. 컴포넌트 명세

### 6.1 Header

| 요소 | 설명 |
|------|------|
| 로고 | 앱 이름 + 아이콘 |
| 언어 선택 | Dropdown (EN/KO/JA/ZH) |
| 테마 토글 | 🌙/☀️ 버튼 (다크/라이트 전환) |
| 오디오 설정 | ⚙️ 버튼 → 모달 오픈 |
| 튜너 | 🎛️ 버튼 → 튜너 오버레이 |
| 프리셋 | 🎵 버튼 → 프리셋 관리 |

### 6.2 GainSlider

**Props:**
- `label`: string (입력/출력)
- `value`: number (dB 값)
- `min`: number (기본 -60)
- `max`: number (기본 +20)
- `onInput`: (value: number) => void

**기능:**
- 수평 슬라이더
- dB 값 실시간 표시
- 더블클릭으로 0dB 리셋

### 6.3 PluginCard

**Props:**
- `plugin`: Plugin 타입
- `isActive`: boolean
- `onToggle`: () => void
- `onEdit`: () => void (사이드패널 오픈)
- `onRemove`: () => void

**상태 표시:**
- `[●]` = 활성화 (파란색)
- `[○]` = 비활성화 (회색)

### 6.4 PluginChain

**기능:**
- 수평 스크롤 (overflow-x-auto)
- 드래그앤드롭으로 순서 변경
- 플러그인 추가 버튼
- 빈 상태: "플러그인이 없습니다" 메시지

### 6.5 PluginEditor (사이드패널)

**토글 방식:**
- 각 PluginCard 의 [Edit] 버튼 클릭 시
- 오른쪽에서 슬라이드인
- 너비: 400px
- 플러그인 이름 표시
- 실제 플러그인 UI 는 iframe 또는 canvas 영역 (목업은 placeholder)

### 6.6 TunerDisplay

**표시 항목:**
- 음높이 게이지 (중앙 정렬 목표)
- 현재 음표 (C, C#, D, 등)
- 주파수 (Hz)
- Detune 표시 (센트 단위)

## 7. 상태 관리 (Stores)

### audio.ts

```typescript
interface AudioState {
  inputGain: number;      // dB
  outputGain: number;     // dB
  inputDevice: string;
  outputDevice: string;
  bufferSize: number;
}
```

### plugins.ts

```typescript
interface Plugin {
  id: string;
  name: string;
  type: 'amp' | 'fx' | 'cab';
  isActive: boolean;
  position: number;
}

interface PluginChainState {
  plugins: Plugin[];
  addPlugin: (plugin: Plugin) => void;
  removePlugin: (id: string) => void;
  reorderPlugins: (from: number, to: number) => void;
  togglePlugin: (id: string) => void;
}
```

### theme.ts

```typescript
interface ThemeState {
  isDark: boolean;
  toggle: () => void;
}
```

### tuner.ts

```typescript
interface TunerState {
  isOpen: boolean;
  frequency: number;      // Hz (목업값)
  note: string;
  cents: number;
  toggle: () => void;
}
```

## 8. 다국어 지원 (i18n)

### 지원 언어

- **en** - English
- **ko** - 한국어
- **ja** - 日本語
- **zh** - 中文 (简体)

### 번역 파일 구조

```json
{
  "app": {
    "title": "Kasane",
    "subtitle": "Guitar Amp & FX Host"
  },
  "audio": {
    "input": "입력",
    "output": "출력",
    "gain": "게인",
    "device": "디바이스"
  },
  "plugins": {
    "chain": "플러그인 체인",
    "add": "추가",
    "edit": "편집",
    "remove": "제거",
    "empty": "플러그인이 없습니다"
  },
  "tuner": {
    "title": "튜너"
  },
  "settings": {
    "title": "오디오 설정",
    "inputDevice": "입력 디바이스",
    "outputDevice": "출력 디바이스",
    "bufferSize": "버퍼 사이즈"
  },
  "theme": {
    "dark": "다크",
    "light": "라이트"
  },
  "language": "언어"
}
```

## 9. 프로토타입 인터랙션

| 액션 | 반응 |
|------|------|
| 테마 토글 클릭 | 즉시 다크/라이트 전환 |
| 언어 선택 | 즉시 UI 텍스트 변경 |
| 슬라이더 드래그 | dB 값 실시간 업데이트 |
| 플러그인 드래그 | 순서 변경 (목업) |
| 플러그인 [●] 클릭 | 온/오프 토글 |
| 플러그인 [Edit] 클릭 | 사이드패널 슬라이드인 |
| 사이드패널 X 클릭 | 사이드패널 슬라이드아웃 |
| 튜너 버튼 클릭 | 튜너 오버레이 표시 |
| 설정 버튼 클릭 | 모달 오픈 |
| 레벨 미터 | 랜덤하게 움직이는 목업 |

## 10. 창 크기 제한

- **최소 크기**: 1024x640 px
- **최대 크기**: 제한 없음
- **종횡비**: 유지하지 않음 (유연한 리사이징)