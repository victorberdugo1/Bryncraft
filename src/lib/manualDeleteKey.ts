import type { KeyboardEvent } from "react";

/**
 * Backspace/Delete manuales para inputs de texto controlados por React.
 *
 * Normalmente el navegador borra el carácter él solo y React solo se entera
 * por el evento "input" que dispara onChange. Si por lo que sea (entorno,
 * IME, captura de eventos de terceros, etc.) eso no llega a pasar, esta
 * función hace el borrado ella misma a partir de la selección actual del
 * input y llama a onChange directamente — no depende de que el navegador
 * "haga su magia".
 */
export function handleManualDeleteKey(
  e: KeyboardEvent<HTMLInputElement>,
  value: string,
  onChange: (next: string) => void,
) {
  if (e.key !== "Delete" && e.key !== "Backspace") return;

  const input = e.currentTarget;
  const start = input.selectionStart ?? value.length;
  const end = input.selectionEnd ?? value.length;

  let next: string;
  let caret: number;

  if (start !== end) {
    // Hay selección: cualquiera de las dos teclas borra el rango seleccionado.
    next = value.slice(0, start) + value.slice(end);
    caret = start;
  } else if (e.key === "Backspace") {
    if (start === 0) return; // nada a la izquierda
    next = value.slice(0, start - 1) + value.slice(start);
    caret = start - 1;
  } else {
    if (end >= value.length) return; // nada a la derecha
    next = value.slice(0, start) + value.slice(end + 1);
    caret = start;
  }

  e.preventDefault();
  onChange(next);
  // El valor del input aún no se ha actualizado (React re-renderiza después),
  // así que colocamos el cursor tras el repintado.
  requestAnimationFrame(() => {
    input.setSelectionRange(caret, caret);
  });
}
