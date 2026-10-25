import React, { ReactNode } from 'react';

// --- Card Components (Shadcn pattern) ---
export function Card({ children, className = '', onClick }: { children: ReactNode; className?: string; onClick?: () => void }) {
  return (
    <div
      onClick={onClick}
      className={`rounded-xl border border-slate-800 bg-slate-900/90 text-slate-100 shadow-lg shadow-black/40 backdrop-blur-md transition-all ${className}`}
    >
      {children}
    </div>
  );
}

export function CardHeader({ children, className = '' }: { children: ReactNode; className?: string }) {
  return (
    <div className={`p-4 border-b border-slate-800/80 flex flex-col gap-1.5 ${className}`}>
      {children}
    </div>
  );
}

export function CardTitle({ children, className = '' }: { children: ReactNode; className?: string }) {
  return (
    <h3 className={`font-semibold text-sm tracking-wide text-slate-100 flex items-center gap-2 ${className}`}>
      {children}
    </h3>
  );
}

export function CardDescription({ children, className = '' }: { children: ReactNode; className?: string }) {
  return (
    <p className={`text-xs text-slate-400 ${className}`}>
      {children}
    </p>
  );
}

export function CardContent({ children, className = '' }: { children: ReactNode; className?: string }) {
  return (
    <div className={`p-4 ${className}`}>
      {children}
    </div>
  );
}

export function CardFooter({ children, className = '' }: { children: ReactNode; className?: string }) {
  return (
    <div className={`p-4 border-t border-slate-800/80 flex items-center justify-between ${className}`}>
      {children}
    </div>
  );
}

// --- Button Component ---
interface ButtonProps extends React.ButtonHTMLAttributes<HTMLButtonElement> {
  variant?: 'primary' | 'secondary' | 'outline' | 'danger' | 'ghost' | 'success';
  size?: 'sm' | 'md' | 'lg';
  icon?: ReactNode;
}

export function Button({
  children,
  variant = 'secondary',
  size = 'md',
  icon,
  className = '',
  disabled,
  ...props
}: ButtonProps) {
  const base = "inline-flex items-center justify-center font-medium rounded-lg transition-all focus:outline-none disabled:opacity-50 disabled:pointer-events-none gap-2";

  const sizeClasses = {
    sm: "px-2.5 py-1 text-xs",
    md: "px-3.5 py-1.5 text-xs font-semibold",
    lg: "px-4 py-2 text-sm font-semibold"
  };

  const variantClasses = {
    primary: "bg-cyan-500 hover:bg-cyan-400 text-slate-950 font-semibold shadow-md shadow-cyan-500/20 active:scale-95",
    secondary: "bg-slate-800 hover:bg-slate-700 text-slate-100 border border-slate-700/80 active:scale-95",
    outline: "border border-slate-700 hover:border-cyan-500 text-slate-300 hover:text-cyan-400 hover:bg-cyan-500/10 active:scale-95",
    danger: "bg-rose-500/20 hover:bg-rose-500/30 text-rose-300 border border-rose-500/40 active:scale-95",
    ghost: "text-slate-400 hover:text-slate-100 hover:bg-slate-800/50",
    success: "bg-emerald-500 hover:bg-emerald-400 text-slate-950 font-semibold shadow-md shadow-emerald-500/20 active:scale-95"
  };

  return (
    <button
      disabled={disabled}
      className={`${base} ${sizeClasses[size]} ${variantClasses[variant]} ${className}`}
      {...props}
    >
      {icon && <span className="shrink-0">{icon}</span>}
      {children}
    </button>
  );
}

// --- Badge Component ---
export function Badge({
  children,
  variant = 'default',
  className = ''
}: {
  children: ReactNode;
  variant?: 'cyan' | 'emerald' | 'amber' | 'rose' | 'purple' | 'default';
  className?: string;
}) {
  const styles = {
    cyan: "bg-cyan-500/10 text-cyan-400 border-cyan-500/30",
    emerald: "bg-emerald-500/10 text-emerald-400 border-emerald-500/30",
    amber: "bg-amber-500/10 text-amber-400 border-amber-500/30",
    rose: "bg-rose-500/10 text-rose-400 border-rose-500/30",
    purple: "bg-purple-500/10 text-purple-400 border-purple-500/30",
    default: "bg-slate-800 text-slate-300 border-slate-700"
  };

  return (
    <span className={`inline-flex items-center px-2 py-0.5 rounded-full text-[10px] font-semibold border font-mono tracking-wider ${styles[variant]} ${className}`}>
      {children}
    </span>
  );
}

// --- Slider Control ---
export function Slider({
  label,
  value,
  min,
  max,
  step = 1,
  unit = '',
  onChange,
  disabled
}: {
  label: string;
  value: number;
  min: number;
  max: number;
  step?: number;
  unit?: string;
  onChange: (val: number) => void;
  disabled?: boolean;
}) {
  return (
    <div className="flex flex-col gap-1.5 text-xs">
      <div className="flex justify-between items-center text-slate-300">
        <span className="font-medium text-slate-300">{label}</span>
        <span className="font-mono text-cyan-400 bg-slate-950 px-1.5 py-0.5 rounded border border-slate-800 text-[11px]">
          {value} {unit}
        </span>
      </div>
      <input
        type="range"
        min={min}
        max={max}
        step={step}
        value={value}
        disabled={disabled}
        onChange={(e) => onChange(parseFloat(e.target.value))}
        className="w-full accent-cyan-400 h-1.5 bg-slate-800 rounded-lg cursor-pointer"
      />
    </div>
  );
}
