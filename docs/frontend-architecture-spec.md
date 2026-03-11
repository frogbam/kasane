# Kasane Frontend Architecture Specification

## 1. Purpose

이 문서는 Kasane의 프론트엔드를 다른 디자인이나 다른 프레임워크로 교체할 때도 유지되어야 하는 구조, 경계, 명세, 계약을 정의한다.

이 문서의 목적은 다음과 같다.

- 프론트엔드 구현체를 `Preact`에서 다른 프레임워크로 교체해도 JUCE 백엔드와의 통신이 깨지지 않게 한다.
- UI 디자인을 전면 교체해도 오디오 호스트 앱으로서의 핵심 동작을 유지하게 한다.
- 새로운 프론트엔드 개발자가 "어디까지가 프론트엔드 책임인지"와 "무엇을 절대 바꾸면 안 되는지"를 빠르게 이해하게 한다.

## 2. Scope

이 문서는 다음 범위를 다룬다.

- 프론트엔드 런타임 경계
- JUCE <-> WebView2 브리지 계약
- 프론트엔드 상태 모델
- 화면 구성 규칙
- 테마, i18n, 접근성, 성능 요구사항
- 다른 프레임워크로 교체할 때의 마이그레이션 체크리스트

이 문서는 다음 범위는 다루지 않는다.

- JUCE 내부 오디오 DSP 알고리즘 구현 상세
- VST3 호스트 내부 그래프 연결 구현 상세
- 장기적인 기능 로드맵의 우선순위 결정

## 3. High-Level Boundary

Kasane는 완전한 웹앱이 아니다. 프론트엔드는 `JUCE desktop app` 내부의 `WebView2`에 렌더링되는 UI 레이어다.

책임 분리는 다음과 같다.

### 3.1 JUCE Backend Responsibilities

- 앱 생명주기
- 메인 윈도우 생성
- 오디오 디바이스 초기화 및 변경
- VST3 스캔
- VST3 인스턴스 생성/삭제/재정렬/바이패스
- 오디오 그래프 유지
- 입력/출력 게인 적용
- 미터 계산
- 튜너 계산
- 플러그인 네이티브 에디터 창 생성
- 설정 저장

### 3.2 Frontend Responsibilities

- 상태 표시
- 사용자 입력 수집
- 백엔드 명령 호출
- 화면 전환 및 오버레이 표시
- 디자인 시스템 적용
- 다국어 텍스트 렌더링
- 브리지 이벤트 수신 후 상태 반영

### 3.3 Forbidden Frontend Responsibilities

프론트엔드는 아래를 직접 구현하면 안 된다.

- 실제 오디오 처리
- 플러그인 스캔 로직
- 플러그인 에디터 임베딩
- 디바이스 제어를 브라우저 API로 직접 수행
- 플러그인 체인의 진실 상태를 프론트 단독으로 소유

즉, 프론트엔드는 `source of truth`가 아니라 `backend-driven UI`여야 한다.

## 4. Current Implementation Reference

현재 구현은 아래 파일들이 기준이다.

- `web/src/main.tsx`
- `web/src/App.tsx`
- `web/src/bridge.ts`
- `web/src/types.ts`
- `web/src/state.ts`
- `Source/MainComponent.cpp`
- `Source/AppState.h`

새 프론트엔드는 현재 파일 구조를 그대로 유지할 필요는 없지만, 이 문서의 계약은 유지해야 한다.

## 5. Runtime Contract

### 5.1 Web Runtime Assumption

- 프론트엔드는 일반 브라우저가 아니라 `JUCE WebBrowserComponent + WebView2` 안에서 동작한다.
- `window.__JUCE__.backend`가 존재한다는 가정 위에서 동작한다.
- 외부 네트워크 요청 없이 로컬 번들만으로 초기 렌더링이 가능해야 한다.

### 5.2 Startup Sequence

프론트엔드는 아래 순서를 따라야 한다.

1. 브리지 초기화
2. backend event listener 등록
3. `frontendReady` 호출
4. `bootstrapState` 또는 `frontendReady` 반환값으로 초기 상태 설정
5. 이후 `audioStateChanged`, `pluginListChanged`, `pluginChainChanged`, `tunerUpdated`, `meterUpdated`, `errorRaised` 이벤트로 상태를 갱신

초기 상태를 프론트가 임의 생성해 고정하면 안 된다.

## 6. Bridge Specification

### 6.1 Global Object

프론트엔드는 아래 객체를 사용한다.

```ts
window.__JUCE__?.backend
```

지원 메서드 계약:

```ts
addEventListener(eventId: string, fn: (payload: unknown) => void): unknown
removeEventListener(token: unknown): void
emitEvent(eventId: string, payload: unknown): void
```

### 6.2 Internal Invoke Event IDs

아래 두 값은 브리지 내부용 예약 식별자다. 바꾸면 안 된다.

- `__juce__invoke`
- `__juce__complete`

### 6.3 Native Command List

아래 명령 이름은 프론트엔드 교체 시에도 그대로 유지해야 한다.

| Command | Params | Expected Behavior |
|---|---|---|
| `frontendReady` | 없음 | 초기 상태 반환 |
| `setLanguage` | `languageCode: string` | 언어 변경 |
| `setTheme` | `themeName: string` | 테마 변경 |
| `setInputGain` | `gainDb: number` | 입력 게인 변경 |
| `setOutputGain` | `gainDb: number` | 출력 게인 변경 |
| `scanPlugins` | 없음 | VST3 스캔 |
| `addPlugin` | `pluginDescriptorId: string` | 플러그인 체인에 추가 |
| `removePlugin` | `chainPluginId: string` | 체인에서 제거 |
| `togglePlugin` | `chainPluginId: string` | 바이패스 토글 |
| `reorderPlugins` | `chainPluginId: string`, `newIndex: number` | 체인 순서 변경 |
| `openPluginEditor` | `chainPluginId: string` | 네이티브 에디터 창 오픈 |
| `openAudioSettings` | 없음 | 최신 오디오 상태 요청 |
| `toggleTuner` | `isOpen: boolean` | 튜너 오버레이 상태 변경 |
| `setAudioDeviceSetup` | `inputDeviceId: string`, `outputDeviceId: string`, `sampleRate: number`, `bufferSize: number` | 오디오 디바이스 설정 적용 |

### 6.4 Backend Event List

아래 이벤트 이름도 계약으로 본다.

| Event | Payload |
|---|---|
| `bootstrapState` | `AppState` |
| `audioStateChanged` | `AudioState` |
| `pluginListChanged` | 현재 구현은 `AppState` 전체 |
| `pluginChainChanged` | 현재 구현은 `AppState` 전체 |
| `tunerUpdated` | `TunerState` |
| `meterUpdated` | `MeterState` |
| `errorRaised` | `string` |

주의:

- 현재 `pluginListChanged`와 `pluginChainChanged`는 `AppState` 전체를 보내고 있다.
- 추후 최적화를 위해 payload를 축소할 수는 있지만, 그 경우 프론트와 백엔드가 함께 바뀌어야 한다.
- 프론트엔드 교체만 하는 작업에서는 기존 payload 계약을 그대로 유지하는 것이 안전하다.

## 7. Shared State Specification

프론트엔드는 아래 타입을 지원해야 한다.

### 7.1 `AppState`

```ts
interface AppState {
  bridgeVersion: string
  language: "en" | "ko" | "ja" | "zh"
  theme: "dark" | "light"
  statusMessage: string
  lastError: string
  isScanningPlugins: boolean
  audio: AudioState
  tuner: TunerState
  meters: MeterState
  availablePlugins: PluginDescriptor[]
  chain: ChainSlot[]
}
```

### 7.2 `AudioState`

```ts
interface AudioState {
  inputGainDb: number
  outputGainDb: number
  audioDeviceType: string
  inputDeviceId: string
  outputDeviceId: string
  inputDeviceName: string
  outputDeviceName: string
  bufferSize: number
  sampleRate: number
  inputDevices: DeviceOption[]
  outputDevices: DeviceOption[]
  bufferSizeOptions: number[]
  sampleRateOptions: number[]
}
```

### 7.3 `PluginDescriptor`

```ts
interface PluginDescriptor {
  id: string
  name: string
  manufacturer: string
  category: string
  format: string
  isEnabled: boolean
}
```

### 7.4 `ChainSlot`

```ts
interface ChainSlot {
  pluginId: string
  name: string
  manufacturer: string
  category: string
  order: number
  bypassed: boolean
}
```

### 7.5 `TunerState`

```ts
interface TunerState {
  isOpen: boolean
  note: string
  frequencyHz: number
  cents: number
  signalLevel: number
}
```

### 7.6 `MeterState`

```ts
interface MeterState {
  inputLeftDb: number
  inputRightDb: number
  outputLeftDb: number
  outputRightDb: number
}
```

## 8. UI Composition Rules

프론트엔드는 반드시 아래 기능 블록을 제공해야 한다.

### 8.1 Header

필수 요소:

- 앱 타이틀
- 언어 선택
- 테마 토글
- 튜너 토글
- 오디오 설정 오픈
- 플러그인 스캔 버튼

### 8.2 Audio Panel

필수 요소:

- 현재 input device 표시
- 현재 output device 표시
- input meter
- output meter
- input gain control
- output gain control

### 8.3 Plugin Chain Panel

필수 요소:

- 사용 가능한 VST3 목록에서 선택
- 플러그인 추가
- 현재 체인 표시
- 체인 재정렬
- 바이패스 토글
- 제거 버튼
- 네이티브 에디터 오픈 버튼

### 8.4 Settings Overlay

필수 요소:

- input device 선택
- output device 선택
- sample rate 선택
- buffer size 선택
- apply 버튼

### 8.5 Tuner Overlay

필수 요소:

- 현재 note
- frequency
- cents
- signal level
- 닫기 동작

## 9. Design Constraints

프레임워크를 바꿔도 아래 제약은 유지해야 한다.

### 9.1 Layout

- 최소 창 크기 `1024x640`
- 데스크톱 우선
- 넓은 화면에서는 `audio + plugin chain` 2단 분할이 가능해야 한다
- 작은 폭에서는 세로 스택으로 무너지더라도 기능 손실이 없어야 한다

### 9.2 Styling

- CSS variable 기반 토큰 유지 권장
- utility framework는 선택 사항이지만 필수는 아님
- 다크/라이트 테마는 즉시 전환 가능해야 한다
- 색만 바뀌는 수준이 아니라 대비와 표면 레이어 구조가 유지되어야 한다

### 9.3 Motion

- 플러그인 재정렬과 오버레이 열기/닫기는 시각적으로 구분되어야 한다
- 과도한 애니메이션은 금지
- 미터와 튜너 업데이트는 빠르되 과도한 re-render를 일으키면 안 된다

## 10. Internationalization Rules

- 지원 언어는 `en`, `ko`, `ja`, `zh`
- UI 문자열은 하드코딩하지 않는다
- note 이름, 숫자 값, dB/Hz 같은 기술 값은 번역 대상이 아니다
- 번역 파일 키는 기능 기준 네임스페이스를 사용한다

권장 예시:

- `app.title`
- `header.settings`
- `audio.inputGain`
- `plugins.remove`
- `settings.bufferSize`

## 11. Accessibility Rules

- 모든 버튼은 텍스트 또는 명시적 `aria-label` 필요
- 키보드만으로 주요 조작이 가능해야 한다
- 플러그인 상태 토글은 시각색상 외에도 텍스트/상태 표현이 필요하다
- 튜너와 미터는 장식 요소가 아니라 현재 상태를 읽을 수 있어야 한다

## 12. Performance Rules

- 미터와 튜너는 고빈도 업데이트이므로 전체 앱 리렌더를 유발하지 않는 구조가 바람직하다
- plugin chain 재정렬은 DOM 전체 재생성을 최소화해야 한다
- 대규모 UI 라이브러리 도입으로 초기 로딩을 무겁게 만들지 말아야 한다
- 초기 렌더는 로컬 번들만으로 완료되어야 한다

## 13. Build Contract

프론트엔드 교체 시에도 아래 조건은 유지되어야 한다.

- 빌드 결과는 정적 번들 이어야 한다
- JUCE가 로컬 파일/리소스 제공 방식으로 로드할 수 있어야 한다
- 런타임에 Node.js가 필요하면 안 된다
- `CMake` 빌드 과정에서 자동으로 프론트엔드 번들이 생성되어야 한다
- 최종 산출물은 `web/dist`와 동등한 정적 파일 구조여야 한다

## 14. Recommended Framework-Agnostic Folder Structure

다른 프레임워크를 쓰더라도 아래 역할 분리는 유지하는 것이 좋다.

```text
web/
  src/
    app/
    bridge/
    features/
      audio/
      plugins/
      tuner/
      settings/
      shell/
    i18n/
    styles/
    types/
    main.(ts|js|tsx|jsx)
```

핵심 원칙:

- `bridge`는 UI 프레임워크 코드와 최대한 분리
- `types`는 브리지 payload 계약의 단일 기준점
- `features`는 기능 단위 분리
- `styles`는 테마 토큰과 레이아웃 규칙 분리

## 15. Migration Rules for a New Frontend Framework

새 프레임워크로 교체할 때 반드시 지켜야 하는 순서는 아래와 같다.

1. 현재 브리지 명령 이름과 이벤트 이름을 변경하지 않는다.
2. `AppState`, `AudioState`, `ChainSlot` 등 payload shape를 유지한다.
3. `frontendReady -> bootstrapState` 초기화 흐름을 유지한다.
4. 플러그인 에디터는 여전히 native window로 열리게 유지한다.
5. 오디오 설정 적용은 반드시 `setAudioDeviceSetup`로 보낸다.
6. 미터와 튜너는 backend event 기반으로만 표시한다.
7. 새 프레임워크 전용 상태 관리 로직은 브리지 레이어 위에 얹는다.
8. 브라우저 단독 개발용 mock bridge를 제공하더라도 실제 계약과 동일한 payload를 사용한다.

## 16. Browser-Only Development Requirement

새 프론트엔드를 개발할 때는 가능하면 `mock bridge`를 제공해야 한다.

mock bridge가 제공해야 하는 것:

- `frontendReady` 응답
- `audioStateChanged` mock 이벤트
- `pluginListChanged` / `pluginChainChanged` mock 데이터
- `tunerUpdated` mock 이벤트
- `meterUpdated` mock 이벤트

이렇게 하면 JUCE 없이도 디자인 및 상호작용 개발이 가능하다.

단, mock은 편의를 위한 것이며 실제 계약을 바꾸면 안 된다.

## 17. Acceptance Checklist

새 프론트엔드가 아래 항목을 모두 만족하면 교체 완료로 본다.

- JUCE 변경 없이 앱이 정상 기동한다
- `frontendReady`로 초기 상태를 받는다
- 언어 전환이 4개 언어에서 정상 동작한다
- 다크/라이트 테마 전환이 동작한다
- input/output gain 조절이 동작한다
- VST3 스캔, 추가, 제거, 바이패스, 순서 변경이 동작한다
- 플러그인 에디터 버튼이 native editor를 연다
- 튜너 오버레이가 동작한다
- 설정 오버레이에서 디바이스 변경이 적용된다
- 오디오 미터가 갱신된다
- backend error가 UI에 표시된다

## 18. What Can Change Freely

아래 항목은 자유롭게 변경 가능하다.

- 프론트엔드 프레임워크
- 상태 관리 라이브러리
- CSS 방법론
- 컴포넌트 분리 방식
- 디자인 언어
- 애니메이션 스타일
- 코드베이스 폴더 구조 세부

## 19. What Must Not Change Without Backend Coordination

아래 항목은 프론트엔드 단독 변경 금지다.

- native command 이름
- backend event 이름
- payload 필드 이름
- payload 자료형
- `pluginId`의 의미
- `inputDeviceId` / `outputDeviceId` 포맷
- `bridgeVersion` 의미
- 플러그인 에디터를 native window로 여는 정책

## 20. Summary

Kasane 프론트엔드는 특정 프레임워크가 아니라 다음 계약 위에 존재해야 한다.

- backend-driven state
- stable JUCE bridge contract
- static local bundle
- multilingual desktop-first UI
- audio host workflow 중심 화면 구성

프레임워크나 디자인은 바꿔도 되지만, 이 계약을 깨면 프론트엔드 교체가 아니라 제품 동작 변경이 된다.
