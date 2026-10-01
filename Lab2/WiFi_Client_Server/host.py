import socket
import threading
import tkinter as tk
from collections import deque
from queue import Empty, Queue

PORT = 8002
MAX_SAMPLES = 200

events = Queue()
history = deque(maxlen=MAX_SAMPLES)
motion_count = 0


def receive_data():
    """Receive newline-separated messages from the STM32."""
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
            server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server.bind(("0.0.0.0", PORT))
            server.listen(1)

            while True:
                events.put(("status", f"Waiting for STM32 on port {PORT}..."))
                connection, address = server.accept()

                with connection:
                    events.put(
                        ("status", f"Connected to STM32 at {address[0]}")
                    )

                    for line in connection.makefile(
                        "r", encoding="utf-8", errors="replace"
                    ):
                        text = line.strip()

                        if text == "MOTION":
                            events.put(("motion", None))
                            continue

                        parts = text.split(",")

                        if len(parts) != 4 or parts[0] != "ACC":
                            continue

                        try:
                            xyz = tuple(map(int, parts[1:]))
                        except ValueError:
                            continue

                        events.put(("sample", xyz))

                events.put(("status", "STM32 disconnected"))

    except OSError as error:
        events.put(("status", f"Network error: {error}"))


def draw_graph():
    """Redraw the accelerometer history."""
    canvas.delete("all")

    # Horizontal reference lines.
    for mg in (-1000, 0, 1000):
        y = 200 - mg * 0.08
        canvas.create_line(40, y, 800, y, fill="#dddddd")
        canvas.create_text(
            20,
            y,
            text=str(mg),
            font=("Arial", 8),
        )

    # Draw X, Y, and Z traces.
    colors = ("red", "green", "blue")

    for axis, color in enumerate(colors):
        points = []

        for index, sample in enumerate(history):
            x = 40 + index * 760 / (MAX_SAMPLES - 1)
            y = 200 - sample[axis] * 0.08
            points.extend((x, y))

        if len(points) >= 4:
            canvas.create_line(*points, fill=color, width=2)


def refresh():
    """Move received events from the network thread into the GUI."""
    global motion_count

    got_sample = False

    while True:
        try:
            kind, value = events.get_nowait()
        except Empty:
            break

        if kind == "status":
            connection_status.config(text=value)

        elif kind == "motion":
            motion_count += 1
            motion_status.config(
                text=f"SIGNIFICANT MOTION DETECTED ({motion_count})",
                fg="red",
            )

        elif kind == "sample":
            history.append(value)
            readings.config(
                text=(
                    f"X: {value[0]:5d} mg    "
                    f"Y: {value[1]:5d} mg    "
                    f"Z: {value[2]:5d} mg"
                )
            )
            got_sample = True

    if got_sample:
        draw_graph()

    window.after(100, refresh)


window = tk.Tk()
window.title("STM32 Accelerometer")
window.resizable(False, False)

connection_status = tk.Label(
    window,
    text="Starting TCP server...",
)
connection_status.pack(pady=(8, 2))

readings = tk.Label(
    window,
    text="X: ----- mg    Y: ----- mg    Z: ----- mg",
    font=("Arial", 12),
)
readings.pack(pady=2)

motion_status = tk.Label(
    window,
    text="No significant-motion event",
    fg="black",
    font=("Arial", 12, "bold"),
)
motion_status.pack(pady=2)

tk.Label(
    window,
    text="X = red     Y = green     Z = blue",
).pack(pady=2)

canvas = tk.Canvas(
    window,
    width=820,
    height=400,
    bg="white",
)
canvas.pack(padx=8, pady=(2, 8))

draw_graph()

threading.Thread(
    target=receive_data,
    daemon=True,
).start()

refresh()
window.mainloop()