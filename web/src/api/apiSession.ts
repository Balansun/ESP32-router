const STORAGE_KEY = "balansun_api_token";

interface LoginResponse {
  ok: boolean;
  token?: string;
}

export function getSessionAuthHeader(): string | undefined {
  try {
    const token = sessionStorage.getItem(STORAGE_KEY);
    if (!token) return undefined;
    return `Bearer ${token}`;
  } catch {
    return undefined;
  }
}

export function setSessionToken(token: string): void {
  try {
    sessionStorage.setItem(STORAGE_KEY, token);
  } catch {
    /* ignore */
  }
}

export function clearSession(): void {
  try {
    sessionStorage.removeItem(STORAGE_KEY);
  } catch {
    /* ignore */
  }
}

export function hasSession(): boolean {
  try {
    return !!sessionStorage.getItem(STORAGE_KEY);
  } catch {
    return false;
  }
}

/** Exchange API password for a session token (stored in sessionStorage). */
export async function verifyLoginPassword(password: string): Promise<boolean> {
  try {
    const res = await fetch("/api/v1/auth/login", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
        Accept: "application/json",
      },
      body: JSON.stringify({ password }),
    });
    if (res.status === 401) return false;
    if (!res.ok) return false;
    const body = (await res.json()) as LoginResponse;
    if (!body.ok) return false;
    if (body.token) setSessionToken(body.token);
    return true;
  } catch {
    return false;
  }
}

/** Check stored token against a protected route without triggering UI redirects. */
export async function probeApiSession(): Promise<boolean> {
  const auth = getSessionAuthHeader();
  if (!auth) return false;
  try {
    const res = await fetch("/api/v1/device", {
      headers: { Accept: "application/json", Authorization: auth },
    });
    return res.ok;
  } catch {
    return false;
  }
}

/** Best-effort server logout, then clear local session. */
export async function logoutSession(): Promise<void> {
  const auth = getSessionAuthHeader();
  try {
    if (auth) {
      await fetch("/api/v1/auth/logout", {
        method: "POST",
        headers: { Accept: "application/json", Authorization: auth },
      });
    }
  } catch {
    /* ignore */
  }
  clearSession();
}
