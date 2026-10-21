import { createContext, createSignal, type ParentComponent, Show, useContext } from "solid-js";

import { Icon } from "../components/icons";
import type { IToastContext } from "../types";

const Toast = (props: { text?: string }) => (
  <div
    role="status"
    class="animate-toast-in fixed left-1/2 z-[60] flex -translate-x-1/2 items-center gap-2.5 rounded-full bg-inverse px-5 py-3 text-[15px] font-semibold whitespace-nowrap text-on-inverse shadow-lg shadow-black/25"
    style={{ top: "max(1rem, var(--safe-top))" }}
  >
    <span class="flex size-6 items-center justify-center rounded-full bg-signal text-on-signal">
      <Icon name="check" size={14} />
    </span>
    <span>{props.text}</span>
  </div>
);

const [toastNotification, setToastNotification] = createSignal<{
  text: string;
  duration: number;
} | null>(null);

let hideTimer: ReturnType<typeof setTimeout> | undefined;

const toast = (text: string, duration: number) => {
  clearTimeout(hideTimer);
  setToastNotification(null);
  // a frame without the toast restarts its animation for back-to-back messages
  requestAnimationFrame(() => {
    setToastNotification({ text, duration });
    hideTimer = setTimeout(() => setToastNotification(null), duration);
  });
};

const ToastContext = createContext<IToastContext>({
  toast,
});

export const ToastProvider: ParentComponent = (props) => {
  return (
    <ToastContext.Provider value={{ toast }}>
      <Show when={!!toastNotification()}>
        <Toast text={toastNotification()?.text} />
      </Show>
      {props.children}
    </ToastContext.Provider>
  );
};

export const useToast = () => useContext(ToastContext);
