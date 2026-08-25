import { Link, useLocation } from "react-router-dom";

const NAV_LINKS = [
  { to: "/", label: "Home" },
  { to: "/about", label: "About" },
  { to: "/faq", label: "FAQ" },
  { to: "/contact", label: "Contact" },
];

export function SiteLayout({ children }: { children: React.ReactNode }) {
  const location = useLocation();

  return (
    <div className="h-full w-full overflow-y-auto bg-background font-ui text-foreground">
      <header className="border-b border-border bg-panel">
        <div className="mx-auto flex max-w-5xl items-center justify-between px-6 py-4">
          <Link to="/" className="flex items-center gap-2">
            <img src="/favicon.svg" alt="" className="h-6 w-6" />
            <span className="text-[15px] font-semibold tracking-tight">Bryncraft</span>
          </Link>
          <nav className="flex items-center gap-1 text-[13px]">
            {NAV_LINKS.map((link) => (
              <Link
                key={link.to}
                to={link.to}
                className={`rounded-md px-3 py-1.5 transition-colors hover:bg-panel-raised ${
                  location.pathname === link.to ? "text-accent" : "text-muted-foreground"
                }`}
              >
                {link.label}
              </Link>
            ))}
            <Link
              to="/studio"
              className="ml-2 rounded-md bg-accent px-3 py-1.5 font-semibold text-background hover:bg-accent-glow"
            >
              Open Studio
            </Link>
          </nav>
        </div>
      </header>

      <main className="mx-auto max-w-5xl px-6 py-12">{children}</main>

      <footer className="border-t border-border">
        <div className="mx-auto flex max-w-5xl flex-col gap-4 px-6 py-8 text-[12px] text-muted-foreground sm:flex-row sm:items-center sm:justify-between">
          <span>© {new Date().getFullYear()} Bryncraft. All rights reserved.</span>
          <div className="flex flex-wrap gap-4">
            <Link to="/about" className="hover:text-foreground">About</Link>
            <Link to="/faq" className="hover:text-foreground">FAQ</Link>
            <Link to="/contact" className="hover:text-foreground">Contact</Link>
            <Link to="/privacy" className="hover:text-foreground">Privacy Policy</Link>
            <Link to="/terms" className="hover:text-foreground">Terms of Service</Link>
          </div>
        </div>
      </footer>
    </div>
  );
}
